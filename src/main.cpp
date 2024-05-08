#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "api/twitch/helix_client.hpp"
#include "bundle.hpp"
#include "commands/command.hpp"
#include "config.hpp"
#include "handlers.hpp"
#include "irc/client.hpp"
#include "irc/message.hpp"
#include "localization/localization.hpp"
#include "stream.hpp"
#include "timer.hpp"

int main(int argc, char *argv[]) {
  std::cout << "hi world\n";

  std::optional<bot::Configuration> o_cfg =
      bot::parse_configuration_from_file(".env");

  if (!o_cfg.has_value()) {
    return -1;
  }

  bot::Configuration cfg = o_cfg.value();

  if (cfg.twitch_credentials.client_id.empty() ||
      cfg.twitch_credentials.token.empty()) {
    std::cerr << "*** TWITCH_CREDENTIALS.CLIENT_ID and "
                 "TWITCH_CREDENTIALS.TOKEN must be set!\n";
    return -1;
  }

  if (cfg.database.name.empty() || cfg.database.user.empty() ||
      cfg.database.password.empty() || cfg.database.host.empty() ||
      cfg.database.port.empty()) {
    std::cerr
        << "*** DB_NAME, DB_USER, DB_PASSWORD, DB_HOST, DB_PORT must be set!\n";
    return -1;
  }

  bot::irc::Client client(cfg.twitch_credentials.client_id,
                          cfg.twitch_credentials.token);
  bot::command::CommandLoader command_loader;
  bot::loc::Localization localization("localization");
  bot::api::twitch::HelixClient helix_client(cfg.twitch_credentials.token,
                                             cfg.twitch_credentials.client_id);

  client.join(client.get_bot_username());

  pqxx::connection conn(GET_DATABASE_CONNECTION_URL(cfg));
  pqxx::work *work = new pqxx::work(conn);

  pqxx::result rows = work->exec(
      "SELECT alias_id FROM channels WHERE opted_out_at is null AND alias_id "
      "!= " +
      std::to_string(client.get_bot_id()));

  std::vector<int> ids;

  for (const auto &row : rows) {
    ids.push_back(row[0].as<int>());
  }

  auto helix_channels = helix_client.get_users(ids);

  // it could be optimized
  for (const auto &helix_channel : helix_channels) {
    auto channel =
        work->exec("SELECT id, alias_name FROM channels WHERE alias_id = " +
                   std::to_string(helix_channel.id));

    if (!channel.empty()) {
      std::string name = channel[0][1].as<std::string>();

      if (name != helix_channel.login) {
        work->exec("UPDATE channels SET alias_name = '" + helix_channel.login +
                   "' WHERE id = " + std::to_string(channel[0][0].as<int>()));
        work->commit();

        delete work;
        work = new pqxx::work(conn);
      }

      client.join(helix_channel.login);
    }
  }

  work->commit();
  delete work;

  conn.close();

  bot::stream::StreamListenerClient stream_listener_client(helix_client, client,
                                                           cfg);

  client.on<bot::irc::MessageType::Privmsg>(
      [&client, &command_loader, &localization, &cfg, &helix_client](
          const bot::irc::Message<bot::irc::MessageType::Privmsg> &message) {
        bot::InstanceBundle bundle{client, helix_client, localization, cfg};

        pqxx::connection conn(GET_DATABASE_CONNECTION_URL(cfg));

        bot::handlers::handle_private_message(bundle, command_loader, message,
                                              conn);

        conn.close();
      });

  client.run();

  std::thread timer_thread(bot::create_timer_thread, &client, &cfg);
  timer_thread.join();

  stream_listener_client.run_thread();

  return 0;
}
