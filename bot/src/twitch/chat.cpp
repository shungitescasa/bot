#include "twitch/chat.hpp"

#include <stdexcept>
#include <string>

#include "cpr/cpr.h"
#include "fmt/format.h"
#include "irc/message.hpp"
#include "ixwebsocket/IXWebSocketMessage.h"
#include "ixwebsocket/IXWebSocketMessageType.h"
#include "logger.hpp"
#include "nlohmann/json.hpp"

namespace bot::twitch {
  void TwitchChatClient::prepare() {
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
                 cpr::Header{{"Authorization", "OAuth " + this->client_token}});

    if (response.status_code != 200) return false;

    nlohmann::json j = nlohmann::json::parse(response.text);
    this->username = j["login"];

    return true;
  }

  void TwitchChatClient::start() {
    log::info("TwitchChatClient",
              "Starting ReadWebsocket for Twitch chat client...");
    this->read_websocket.start();
  }

  void TwitchChatClient::say(unsigned int channel_id,
                             const std::string &message) {
    if (this->websocket_session_id.empty()) {
      throw std::runtime_error("websocket_session_id is not set yet!");
    }

    log::debug("TwitchChatClient",
               fmt::format("Sending \"{}\" to {}...", message, channel_id));

    nlohmann::json j = {{"broadcaster_id", channel_id},
                        {"sender_id", this->user_id},
                        {"message", message}};

    cpr::Response response =
        cpr::Post(cpr::Url{"https://api.twitch.tv/helix/chat/messages"},
                  cpr::Header{{"Authorization", "Bearer " + this->client_token},
                              {"Client-Id", this->client_id},
                              {"Content-Type", "application/json"}},
                  cpr::Body{j.dump()});

    if (response.status_code != 200) {
      throw std::runtime_error(
          fmt::format("Failed to send chat message! API returned {} code.",
                      response.status_code));
    }
  }

  void TwitchChatClient::say(const std::string &channel_login,
                             const std::string &message) {
    cpr::Response response = cpr::Get(cpr::Url{fmt::format(
        "https://api.ivr.fi/v2/twitch/user?login={}", channel_login)});

    if (response.status_code != 200) {
      return;
    }

    nlohmann::json j = nlohmann::json::parse(response.text);

    this->say(std::stoi((std::string)j[0]["id"]), message);
  }

  void TwitchChatClient::join(unsigned int channel_id) {
    if (this->websocket_session_id.empty()) {
      throw std::runtime_error("websocket_session_id is not set yet!");
    }

    log::info("TwitchChatClient", fmt::format("Joining {}...", channel_id));

    using namespace nlohmann::literals;

    nlohmann::json j = {{"type", "channel.chat.message"},
                        {"version", "1"},
                        {"condition",
                         {{"broadcaster_user_id", std::to_string(channel_id)},
                          {"user_id", std::to_string(this->user_id)}}},
                        {"transport",
                         {{"method", "websocket"},
                          {"session_id", this->websocket_session_id}}}};

    cpr::Response response = cpr::Post(
        cpr::Url{"https://api.twitch.tv/helix/eventsub/subscriptions"},
        cpr::Header{{"Authorization", "Bearer " + this->client_token},
                    {"Client-Id", this->client_id},
                    {"Content-Type", "application/json"}},
        cpr::Body{j.dump()});

    if (response.status_code != 202) {
      log::info("nah", response.text);
      throw std::runtime_error(
          fmt::format("Failed to subscribe to channel.chat.message! API "
                      "returned {} code.",
                      response.status_code));
    }
  }

  void TwitchChatClient::join(const std::string &channel_login) {
    cpr::Response response = cpr::Get(cpr::Url{fmt::format(
        "https://api.ivr.fi/v2/twitch/user?login={}", channel_login)});

    if (response.status_code != 200) {
      return;
    }

    nlohmann::json j = nlohmann::json::parse(response.text);

    this->join(std::stoi((std::string)j[0]["id"]));
  }

  const unsigned int &TwitchChatClient::get_user_id() const {
    return this->user_id;
  }

  const std::string &TwitchChatClient::get_username() const {
    return this->username;
  }

  void TwitchChatClient::handleWebsocketMessage(const std::string &raw) {
    nlohmann::json j = nlohmann::json::parse(raw);

    std::string message_type = j["metadata"]["message_type"];

    if (message_type == "session_welcome") {
      this->websocket_session_id = j["payload"]["session"]["id"];
      this->onConnect();
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