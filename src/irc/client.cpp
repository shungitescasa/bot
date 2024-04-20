#include "client.hpp"

#include <ixwebsocket/IXWebSocketMessage.h>
#include <ixwebsocket/IXWebSocketMessageType.h>

#include <iostream>
#include <string>

using namespace RedpilledBot::IRC;

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
            break;
          }
          case ix::WebSocketMessageType::Open: {
            std::cout << "Connected to Twitch IRC!\n";
            break;
          }
          case ix::WebSocketMessageType::Close: {
            std::cout << "Twitch IRC Connection closed!\n";
            break;
          }
          default: {
            break;
          }
        }
      });

  this->websocket.run();
}
