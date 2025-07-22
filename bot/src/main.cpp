#include <emotespp/seventv.hpp>
#include <map>
#include <memory>
#include <optional>
#include <sol/state.hpp>
#include <string>
#include <thread>
#include <vector>

#include "api/kick.hpp"
#include "api/twitch/helix_client.hpp"
#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/lua.hpp"
#include "commands/response.hpp"
#include "config.hpp"
#include "database.hpp"
#include "emotes.hpp"
#include "github.hpp"
#include "handlers.hpp"
#include "irc/client.hpp"
#include "irc/message.hpp"
#include "localization/localization.hpp"
#include "logger.hpp"
#include "rss.hpp"
#include "schemas/stream.hpp"
#include "stream.hpp"
#include "timer.hpp"

int main(int argc, char *argv[]) {
  bot::log::info("Main", "Starting up...");

  std::optional<bot::Configuration> o_cfg =
      bot::parse_configuration_from_file(".env");

  if (!o_cfg.has_value()) {
    return 1;
  }

  bot::Configuration cfg = o_cfg.value();

  if (cfg.twitch_credentials.client_id.empty() ||
      cfg.twitch_credentials.token.empty()) {
    bot::log::error("Main",
                    "TWITCH_CREDENTIALS.CLIENT_ID and TWITCH_CREDENTIALS.TOKEN "
                    "must be set in environmental file!");
    return 1;
  }

  if (cfg.database.name.empty() || cfg.database.user.empty() ||
      cfg.database.password.empty() || cfg.database.host.empty() ||
      cfg.database.port.empty()) {
    bot::log::error("Main",
                    "DB_NAME, DB_USER, DB_PASSWORD, DB_HOST, DB_PORT "
                    "must be set in environmental file!");
    return 1;
  }

  bot::irc::Client client(cfg.twitch_credentials.client_id,
                          cfg.twitch_credentials.token);
  bot::command::CommandLoader command_loader;
  command_loader.load_lua_directory("luamods");

  bot::loc::Localization localization("localization");
  bot::api::twitch::HelixClient helix_client(cfg.twitch_credentials.token,
                                             cfg.twitch_credentials.client_id);

  bot::api::KickAPIClient kick_api_client(cfg.kick_credentials.client_id,
                                          cfg.kick_credentials.client_secret);

#ifdef BUILD_BETTERTTV
  emotespp::BetterTTVWebsocketClient bttv_ws_client;
#endif
  emotespp::SevenTVWebsocketClient seventv_emote_listener;
  emotespp::SevenTVAPIClient seventv_api_client;

  client.join(client.get_bot_username());

  std::unique_ptr<bot::db::BaseDatabase> conn = bot::db::create_connection(cfg);

  bot::db::DatabaseRows rows = conn->exec(
      "SELECT alias_id FROM channels WHERE opted_out_At IS NULL AND alias_id "
      "!= "
      "$1",
      {std::to_string(client.get_bot_id())});

  std::vector<int> ids;

  for (const bot::db::DatabaseRow &row : rows) {
    ids.push_back(std::stoi(row.at("alias_id")));
  }

  auto helix_channels = helix_client.get_users(ids);

  // it could be optimized
  for (const auto &helix_channel : helix_channels) {
    std::vector<std::map<std::string, std::string>> channels =
        conn->exec("SELECT id, alias_name FROM channels WHERE alias_id = $1",
                   {std::to_string(helix_channel.id)});

    if (!channels.empty()) {
      std::string name = channels[0]["alias_name"];

      if (name != helix_channel.login) {
        conn->exec("UPDATE channels SET alias_name = $1 WHERE id = $2",
                   {helix_channel.login, channels[0][0]});
      }

      client.join(helix_channel.login);
    }
  }

  conn->close();

  bot::stream::StreamListenerClient stream_listener_client(
      helix_client, kick_api_client, client, cfg);

  bot::GithubListener github_listener(cfg, client, helix_client);

  bot::emotes::EmoteEventBundle emote_bundle{client,
                                             helix_client,
#ifdef BUILD_BETTERTTV
                                             bttv_ws_client,
#endif
                                             seventv_emote_listener,
                                             seventv_api_client,
                                             cfg};

  // ---------------
  // 7TV !!!
  // ---------------
  seventv_emote_listener.on_emote_create(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::STV_EMOTE_CREATE,
                                        channel_name, author_id, emote);
      });

  seventv_emote_listener.on_emote_delete(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::STV_EMOTE_DELETE,
                                        channel_name, author_id, emote);
      });

  seventv_emote_listener.on_emote_update(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::STV_EMOTE_UPDATE,
                                        channel_name, author_id, emote);
      });

  seventv_emote_listener.start();

#ifdef BUILD_BETTERTTV
  // ----------------
  // BETTERTTV
  // ----------------

  bttv_ws_client.on_emote_create(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::BTTV_EMOTE_CREATE,
                                        channel_name, author_id, emote);
      });

  bttv_ws_client.on_emote_delete(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::BTTV_EMOTE_DELETE,
                                        channel_name, author_id, emote);
      });

  bttv_ws_client.on_emote_update(
      [&emote_bundle](const std::string &channel_name,
                      const std::optional<std::string> &author_id,
                      const emotespp::Emote &emote) {
        bot::emotes::handle_emote_event(emote_bundle,
                                        bot::schemas::BTTV_EMOTE_UPDATE,
                                        channel_name, author_id, emote);
      });

  bttv_ws_client.start();

#endif

  bot::RSSListener rss_listener(client, helix_client, cfg);

  client.on<bot::irc::MessageType::Privmsg>(
      [&client, &command_loader, &localization, &cfg, &helix_client,
       &kick_api_client](
          const bot::irc::Message<bot::irc::MessageType::Privmsg> &message) {
        bot::InstanceBundle bundle{client,       helix_client, kick_api_client,
                                   localization, cfg,          command_loader};

        bot::handlers::handle_private_message(bundle, command_loader, message);
      });

  client.run();

  std::vector<std::thread> threads;
  threads.push_back(std::thread(bot::create_timer_thread, &client, &cfg));
  threads.push_back(std::thread(&bot::stream::StreamListenerClient::run,
                                &stream_listener_client));
  threads.push_back(std::thread(&bot::GithubListener::run, &github_listener));
  threads.push_back(
      std::thread(bot::emotes::create_emote_thread, &emote_bundle));
  threads.push_back(std::thread(&bot::api::KickAPIClient::refresh_token_thread,
                                &kick_api_client));
  threads.push_back(std::thread(&bot::RSSListener::run, &rss_listener));

  for (auto &thread : threads) {
    thread.join();
  }

  return 0;
}
