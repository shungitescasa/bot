#include "client.hpp"

#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketMessageType.h>

#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "message.hpp"

using namespace bot::irc;

Client::Client(std::string username, std::string password) {
  this->username = username;
  this->password = password;

  this->host = "wss://irc-ws.chat.twitch.tv";
  this->port = "443";

  this->websocket.setUrl(this->host + ":" + this->port);
}

void Client::run() {
  this->websocket.setOnMessageCallback(
      [this](const ix::WebSocketMessagePtr &msg) {
        switch (msg->type) {
          case ix::WebSocketMessageType::Message: {
            std::cout << "Got a message: " << msg->str << std::endl;

            std::vector<std::string> lines =
                utils::string::split_text(msg->str, '\n');

            for (std::string &line : lines) {
              line.erase(std::remove_if(line.begin(), line.end(),
                                        [](char c) {
                                          return c == '\n' || c == '\r' ||
                                                 c == '\t';
                                        }),
                         line.end());

              std::optional<MessageType> type = define_message_type(line);

              if (!type.has_value()) {
                break;
              }

              MessageType m_type = type.value();

              if (m_type == MessageType::Privmsg) {
                std::optional<Message<MessageType::Privmsg>> message =
                    parse_message<MessageType::Privmsg>(line);

                if (message.has_value()) {
                  this->onPrivmsg(message.value());
                }
              }
            }

            break;
          }
          case ix::WebSocketMessageType::Open: {
            std::cout << "Connected to Twitch IRC!\n";
            this->is_connected = true;
            this->authorize();
            for (const auto &msg : this->pool) {
              this->websocket.send(msg);
            }
            this->pool.clear();
            break;
          }
          case ix::WebSocketMessageType::Close: {
            std::cout << "Twitch IRC Connection closed!\n";
            this->is_connected = false;
            break;
          }
          default: {
            break;
          }
        }
      });

  this->websocket.run();
}

void Client::raw(const std::string &raw_message) {
  std::string msg = raw_message + "\r\n";
  if (this->is_connected) {
    this->websocket.send(msg);
  } else {
    this->pool.push_back(msg);
  }
}

void Client::authorize() {
  if (this->username.empty() || this->password.empty()) {
    std::cout << "Bot username and password must be set!\n";
    return;
  }

  std::cout << "Authorizing on Twitch IRC servers...\n";

  this->raw("PASS " + this->password);
  this->raw("NICK " + this->username);
  this->raw("CAP REQ :twitch.tv/membership");
  this->raw("CAP REQ :twitch.tv/commands");
  this->raw("CAP REQ :twitch.tv/tags");
}
