#include <chrono>
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
#include "config.hpp"
#include "database.hpp"
#include "emotes.hpp"
#include "github.hpp"
#include "handlers.hpp"
#include "irc/message.hpp"
#include "localization/localization.hpp"
#include "logger.hpp"
#include "rss.hpp"
#include "schemas/stream.hpp"
#include "stream.hpp"
#include "timer.hpp"
#ifdef USE_EVENTSUB_CONNECTION
#include "twitch/chat.hpp"
#else
#include "irc/client.hpp"
#endif

int main(int argc, char *argv[]) {
  bot::log::info("Main", "Starting up...");

  std::optional<bot::Configuration> o_cfg =
      bot::parse_configuration_from_file(".env");

  if (!o_cfg.has_value()) {
    return 1;
  }

  bot::Configuration cfg = o_cfg.value();

  if (cfg.twitch.client_id.empty() || cfg.twitch.token.empty()) {
    bot::log::error("Main",
                    "TWITCH.CLIENT_ID and TWITCH.TOKEN "
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

#ifdef USE_EVENTSUB_CONNECTION
  bot::twitch::TwitchChatClient twitch_client(
      cfg.twitch.user_id, cfg.twitch.token, cfg.twitch.client_id);
#else
  bot::irc::Client twitch_client(cfg.twitch.client_id, cfg.twitch.token);
#endif
  bot::command::CommandLoader command_loader;
  command_loader.load_lua_directory("luascripts");

  bot::loc::Localization localization("localization");
  bot::api::twitch::HelixClient helix_client(cfg.twitch.token,
                                             cfg.twitch.client_id);

  bot::api::KickAPIClient kick_api_client(cfg.kick_credentials.client_id,
                                          cfg.kick_credentials.client_secret);

#ifdef BUILD_BETTERTTV
  emotespp::BetterTTVWebsocketClient bttv_ws_client;
#endif
  emotespp::SevenTVWebsocketClient seventv_emote_listener;
  emotespp::SevenTVAPIClient seventv_api_client;

  std::unique_ptr<bot::db::BaseDatabase> conn = bot::db::create_connection(cfg);

  bot::db::DatabaseRows id_rows = conn->exec(
      "SELECT alias_id, alias_name FROM channels WHERE opted_out_at IS NULL "
      "AND alias_id "
      "!= "
      "$1",
      {std::to_string(twitch_client.get_me().id)});

  conn->close();

  bot::stream::StreamListenerClient stream_listener_client(
      helix_client, kick_api_client, twitch_client, cfg);

  bot::GithubListener github_listener(cfg, twitch_client, helix_client);

  bot::emotes::EmoteEventBundle emote_bundle{
      twitch_client,          helix_client,
#ifdef BUILD_BETTERTTV
      bttv_ws_client,
#endif
      seventv_emote_listener, seventv_api_client, cfg};

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

  bot::RSSListener rss_listener(twitch_client, helix_client, cfg);

  twitch_client.on_connect([&twitch_client, &id_rows]() {
    bot::log::info("Main", "Joining channels...");
    twitch_client.join(twitch_client.get_me());
    for (const bot::db::DatabaseRow &row : id_rows) {
      twitch_client.join({row.at("alias_name"), std::stoi(row.at("alias_id"))});
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  });

  twitch_client.on_privmsg(
      [&twitch_client, &command_loader, &localization, &cfg, &helix_client,
       &kick_api_client](
          const bot::irc::Message<bot::irc::MessageType::Privmsg> &message) {
        bot::InstanceBundle bundle{twitch_client, helix_client, kick_api_client,
                                   localization,  cfg,          command_loader};
        bot::handlers::handle_private_message(bundle, command_loader, message);
      });

  twitch_client.run();

  std::vector<std::thread> threads;
  threads.push_back(
      std::thread(bot::create_timer_thread, &twitch_client, &cfg));
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
