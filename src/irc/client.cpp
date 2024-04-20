#include "client.hpp"

#include <string>

using namespace RedpilledBot::IRC;

Client::Client(std::string username, std::string password) {
  this->username = username;
  this->password = password;
}
