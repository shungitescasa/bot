#include "client.hpp"

#include <string>

using namespace RedpilledBot::IRC;

Client::Client(std::string username, std::string password) {
  this->username = username;
  this->password = password;

  this->host = "wss://irc-ws.chat.twitch.tv";
  this->port = "443";

  this->websocket.setUrl(this->host + ":" + this->port);
}
