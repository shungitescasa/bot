#include <algorithm>
#include <ctime>
#include <optional>

#include "ixwebsocket/IXWebSocket.h"
#ifdef USE_EVENTSUB_CONNECTION
#include <stdexcept>
#include <string>

#include "cpr/cpr.h"
#include "fmt/format.h"
#include "irc/message.hpp"
#include "ixwebsocket/IXWebSocketMessage.h"
#include "ixwebsocket/IXWebSocketMessageType.h"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "twitch/chat.hpp"

namespace bot::twitch {
  void TwitchChatClient::authorize_app() {
    cpr::Response response = cpr::Post(cpr::Url{fmt::format(
        "https://id.twitch.tv/oauth2/"
        "token?grant_type=client_credentials&client_id={}&client_secret={}",
        this->app_client_id, this->app_client_secret)});

    if (response.status_code != 200) {
      throw std::runtime_error(fmt::format("Failed to get app access token: {}",
                                           response.status_code));
    }

    nlohmann::json j = nlohmann::json::parse(response.text);

    this->app_token = j["access_token"];
    this->app_token_acquisition = std::time(nullptr);
    this->app_token_expiration =
        this->app_token_acquisition + (unsigned int)j["expires_in"];

    log::info("TwitchChatClient", "Acquired app access token!");
  }

  void TwitchChatClient::prepare() {
    this->authorize_app();

    if (!this->validate_token()) {
      throw std::runtime_error("Invalid token.");
    }

    this->read_websocket.setUrl("wss://eventsub.wss.twitch.tv/ws");
    this->read_websocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr &msg) {
          switch (msg->type) {
            case ix::WebSocketMessageType::Message:
              this->handleWebsocketMessage(msg->str);
              break;
            case ix::WebSocketMessageType::Error:
              log::error("TwitchChatClient",
                         fmt::format("Error occurred on ReadWebsocket: {} {}",
                                     msg->errorInfo.http_status,
                                     msg->errorInfo.reason));
              break;
            case ix::WebSocketMessageType::Open:
              log::info("TwitchChatClient", "Connected to Twitch EventSub!");
              break;
            default:
              break;
          }
        });
  }

  bool TwitchChatClient::validate_token() {
    cpr::Response response =
        cpr::Get(cpr::Url{"https://id.twitch.tv/oauth2/validate"},
                 cpr::Header{{"Authorization", "OAuth " + this->user_token}});

    if (response.status_code != 200) return false;

    nlohmann::json j = nlohmann::json::parse(response.text);
    this->me.login = j["login"];
    this->me.id = this->user_id;

    return true;
  }

  void TwitchChatClient::run() {
    log::info("TwitchChatClient",
              "Starting ReadWebsocket for Twitch chat client...");
    this->read_websocket.start();
  }

  void TwitchChatClient::say(const irc::MessageSource &room,
                             const std::string &message) {
    if (this->app_token.empty()) {
      throw std::runtime_error("app_token is not set yet!");
    }

    log::debug("TwitchChatClient",
               fmt::format("Sending \"{}\" to {}...", message, room.id));

    nlohmann::json j = {{"broadcaster_id", room.id},
                        {"sender_id", this->user_id},
                        {"message", message}};

    cpr::Response response =
        cpr::Post(cpr::Url{"https://api.twitch.tv/helix/chat/messages"},
                  cpr::Header{{"Authorization", "Bearer " + this->app_token},
                              {"Client-Id", this->app_client_id},
                              {"Content-Type", "application/json"}},
                  cpr::Body{j.dump()});

    if (response.status_code != 200) {
      throw std::runtime_error(
          fmt::format("Failed to send chat message! API returned {} code.",
                      response.status_code));
    }

    // that's the most secure piece of code in this project
    j = nlohmann::json::parse(response.text);
    if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
      auto msg = j["data"][0];

      if (msg.contains("is_sent") && msg["is_sent"].is_boolean() &&
          !((bool)msg["is_sent"])) {
        std::string code = "unknown";
        std::string reason = "Failed to send a message. Try again later!";

        if (msg.contains("drop_reason") && !msg["drop_reason"].is_null()) {
          auto r = msg["drop_reason"];

          if (r.contains("code") && r["code"].is_string()) {
            code = r["code"];
          }

          if (r.contains("message") && r["message"].is_string()) {
            reason = r["message"];
          }
        }

        log::info("TwitchChatClient",
                  fmt::format("Failed to send a message to room ID {}: {} ({}) "
                              "(message - {})",
                              room.id, reason, code, message));

        if (reason != message) {
          this->say(room, reason);
        }
      }
    }
  }

  void TwitchChatClient::join(const irc::MessageSource &room) {
    if (this->websocket_session_id.empty()) {
      throw std::runtime_error("websocket_session_id is not set yet!");
    }

    log::info("TwitchChatClient", fmt::format("Joining {}...", room.id));
    this->joined_channels.insert_or_assign(room.id, std::nullopt);

    nlohmann::json j = {{"type", "channel.chat.message"},
                        {"version", "1"},
                        {"condition",
                         {{"broadcaster_user_id", std::to_string(room.id)},
                          {"user_id", std::to_string(this->user_id)}}},
                        {"transport",
                         {{"method", "websocket"},
                          {"session_id", this->websocket_session_id}}}};

    cpr::Response response = cpr::Post(
        cpr::Url{"https://api.twitch.tv/helix/eventsub/subscriptions"},
        cpr::Header{{"Authorization", "Bearer " + this->user_token},
                    {"Client-Id", this->user_client_id},
                    {"Content-Type", "application/json"}},
        cpr::Body{j.dump()});

    if (response.status_code != 202) {
      log::info("nah", response.text);
      throw std::runtime_error(
          fmt::format("Failed to subscribe to channel.chat.message! API "
                      "returned {} code.",
                      response.status_code));
    }

    j = nlohmann::json::parse(response.text);

    std::string id = j["data"][0]["id"];
    this->joined_channels.insert_or_assign(room.id, id);
    log::info("TwitchChatClient",
              fmt::format("Joined #{} (ID {})!", room.login, id));
  }

  void TwitchChatClient::part(const irc::MessageSource &room) {
    if (!std::any_of(this->joined_channels.begin(), this->joined_channels.end(),
                     [&room](const auto &x) { return x.first == room.id; })) {
      log::error("TwitchChatClient",
                 fmt::format("Room ID {} is not joined!", room.id));
      return;
    }

    std::optional<std::string> id = this->joined_channels.at(room.id);
    if (!id.has_value()) {
      log::error(
          "TwitchChatClient",
          fmt::format("Room ID {} does not have subscription ID!", room.id));
      return;
    }

    cpr::Response response = cpr::Delete(
        cpr::Url{"https://api.twitch.tv/helix/eventsub/subscriptions?id=" +
                 *id},
        cpr::Header{{"Authorization", "Bearer " + this->user_token},
                    {"Client-Id", this->user_client_id},
                    {"Content-Type", "application/json"}});

    if (response.status_code != 204) {
      log::info("nah", response.text);
      throw std::runtime_error(
          fmt::format("Failed to unsubscribe from channel.chat.message! API "
                      "returned {} code.",
                      response.status_code));
    }

    this->joined_channels.erase(room.id);

    log::info("TwitchChatClient", fmt::format("Parted #{}!", room.login));
  }

  void TwitchChatClient::handleWebsocketMessage(const std::string &raw) {
    nlohmann::json j = nlohmann::json::parse(raw);

    std::string message_type = j["metadata"]["message_type"];

    if (message_type == "session_welcome") {
      this->websocket_session_id = j["payload"]["session"]["id"];
      this->onConnect();
    } else if (message_type == "session_reconnect") {
      if (this->read_websocket.getReadyState() == ix::ReadyState::Open) {
        this->read_websocket.close();
      }

      auto s = j["payload"]["session"];

      this->read_websocket.setUrl(s["reconnect_url"]);
      this->websocket_session_id = s["id"];
      this->read_websocket.start();
    } else if (message_type == "notification") {
      std::string subtype = j["metadata"]["subscription_type"];

      if (subtype == "channel.chat.message") {
        nlohmann::json e = j["payload"]["event"];

        irc::Message<irc::Privmsg> message;

        // parsing source
        irc::MessageSource source;
        source.id = std::stoi((std::string)e["broadcaster_user_id"]);
        source.login = e["broadcaster_user_login"];

        // parsing sender
        irc::MessageSender sender;
        sender.id = std::stoi((std::string)e["chatter_user_id"]);
        sender.login = e["chatter_user_login"];
        sender.display_name = e["chatter_user_name"];

        for (const nlohmann::json &b : e["badges"]) {
          sender.badges.insert_or_assign(b["set_id"], b["id"]);
        }

        message.sender = sender;
        message.source = source;
        message.message = e["message"]["text"];

        this->onPrivmsg(message);
      }
    }
  }
}
#endif