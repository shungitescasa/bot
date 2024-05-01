#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "api/twitch/helix_client.hpp"
#include "bundle.hpp"
#include "commands/command.hpp"
#include "config.hpp"
#include "handlers.hpp"
#include "irc/client.hpp"
#include "irc/message.hpp"
#include "localization/localization.hpp"

int main(int argc, char *argv[]) {
  std::cout << "hi world\n";

  std::optional<bot::Configuration> o_cfg =
      bot::parse_configuration_from_file(".env");

  if (!o_cfg.has_value()) {
    return -1;
  }

  bot::Configuration cfg = o_cfg.value();

  if (cfg.bot_password.empty() || cfg.bot_username.empty() ||
      cfg.bot_client_id.empty()) {
    std::cerr
        << "*** BOT_USERNAME, BOT_CLIENT_ID and BOT_PASSWORD must be set!\n";
    return -1;
  }

  if (cfg.database.name.empty() || cfg.database.user.empty() ||
      cfg.database.password.empty() || cfg.database.host.empty() ||
      cfg.database.port.empty()) {
    std::cerr
        << "*** DB_NAME, DB_USER, DB_PASSWORD, DB_HOST, DB_PORT must be set!\n";
    return -1;
  }

  bot::irc::Client client(cfg.bot_username, cfg.bot_password);
  bot::command::CommandLoader command_loader;
  bot::loc::Localization localization("localization");

  client.join(cfg.bot_username);

  pqxx::connection conn(GET_DATABASE_CONNECTION_URL(cfg));
  pqxx::work work(conn);

  auto rows = work.exec("SELECT alias_name FROM channels");

  for (const auto &row : rows) {
    auto name = row[0].as<std::string>();
    client.join(name);
  }

  work.commit();
  conn.close();

  bot::api::twitch::HelixClient helix_client(cfg.bot_password,
                                             cfg.bot_client_id);

  client.on<bot::irc::MessageType::Privmsg>(
      [&client, &command_loader, &localization,
       &cfg](const bot::irc::Message<bot::irc::MessageType::Privmsg> &message) {
        bot::InstanceBundle bundle{client, localization};

        pqxx::connection conn(GET_DATABASE_CONNECTION_URL(cfg));

        bot::handlers::handle_private_message(bundle, command_loader, message,
                                              conn);

        conn.close();
      });

  client.run();

  return 0;
}
