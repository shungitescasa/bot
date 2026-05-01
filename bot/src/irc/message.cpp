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

  std::optional<IRCMessage> IRCMessage::from_string(std::string msg) {
    IRCMessage m;

    if (msg.empty()) return std::nullopt;

    if (msg[0] == '@') {
      size_t end = msg.find(' ');
      if (end == std::string::npos) return std::nullopt;

      std::string raw_tags = msg.substr(1, end - 1);

      for (const std::string &tag : utils::string::split_text(raw_tags, ';')) {
        auto splitted = utils::string::split_text_n(tag, "=", 1);
        if (splitted.size() == 2)
          m.tags.insert_or_assign(splitted[0], splitted[1]);
      }

      msg = msg.substr(end + 1);
      if (msg.empty()) return std::nullopt;
    }

    if (msg[0] == ':') {
      size_t end = msg.find(' ');
      if (end == std::string::npos) return std::nullopt;

      m.prefix = msg.substr(1, end - 1);

      size_t bang = m.prefix.find('!');
      if (bang != std::string::npos) m.nick = m.prefix.substr(0, bang);

      msg = msg.substr(end + 1);
      if (msg.empty()) return std::nullopt;
    }

    auto parts = utils::string::split_text(msg, ' ');
    if (parts.empty()) return std::nullopt;

    m.command = parts.front();
    parts.erase(parts.begin());

    for (size_t i = 0; i < parts.size(); i++) {
      if (!parts[i].empty() && parts[i][0] == ':') {
        std::vector<std::string> trailing(parts.begin() + i, parts.end());
        m.params.push_back(utils::string::join_vector(trailing, ' ').substr(1));
        break;
      }
      if (!parts[i].empty()) m.params.push_back(parts[i]);
    }

    return m;
  }
}
