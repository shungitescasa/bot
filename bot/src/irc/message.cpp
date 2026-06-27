#include "message.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bot::irc {
  MessageSource::MessageSource(const sol::table &table) {
    this->id = table["id"];
    this->login = table["login"];
  }

  MessageSource::MessageSource(const std::string &login, const int &id) {
    this->id = id;
    this->login = login;
  }

  std::optional<MessageType> define_message_type(const std::string &msg) {
    if (msg == "NOTICE") {
      return MessageType::Notice;
    } else if (msg == "PRIVMSG") {
      return MessageType::Privmsg;
    } else if (msg == "PING") {
      return MessageType::Ping;
    } else if (msg == "001") {
      return MessageType::Connect;
    }

    return std::nullopt;
  }
}
