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
            this->authorize();
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

void Client::authorize() {
  if (this->username.empty() || this->password.empty()) {
    std::cout << "Bot username and password must be set!\n";
    return;
  }

  std::cout << "Authorizing on Twitch IRC servers...\n";

  this->websocket.send("PASS " + this->password + "\r\n");
  this->websocket.send("NICK " + this->username + "\r\n");
  this->websocket.send("CAP REQ :twitch.tv/membership\r\n");
  this->websocket.send("CAP REQ :twitch.tv/commands\r\n");
  this->websocket.send("CAP REQ :twitch.tv/tags\r\n");
}
