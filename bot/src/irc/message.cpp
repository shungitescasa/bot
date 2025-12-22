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
    }

    return std::nullopt;
  }

  std::optional<IRCMessage> IRCMessage::from_string(std::string msg) {
    IRCMessage m;

    if (msg[0] == '@') {
      int end = msg.find(' ');
      std::string raw_tags = msg.substr(1, end);
      for (const std::string tag : utils::string::split_text(raw_tags, ';')) {
        auto splitted_tag = utils::string::split_text_n(tag, "=", 1);
        m.tags.insert_or_assign(splitted_tag[0], splitted_tag[1]);
      }
      msg = msg.substr(end + 1);
    }

    if (msg[0] == ':') {
      int end = msg.find(' ');
      m.prefix = msg.substr(1, end);
      int bang = msg.find('!');
      if (bang != std::string::npos) {
        m.nick = m.prefix.substr(0, bang - 1);
      }
      msg = msg.substr(end + 1);
    }

    std::vector<std::string> parts = utils::string::split_text(msg, ' ');

    m.command = parts.front();
    parts.erase(parts.begin());

    for (int i = 0; i < parts.size(); i++) {
      if (parts[i][0] == ':') {
        parts = std::vector<std::string>(parts.begin() + i, parts.end());
        m.params.push_back(utils::string::join_vector(parts, ' ').substr(1));
        break;
      }
      if (!parts[i].empty()) {
        m.params.push_back(parts[i]);
      }
    }

    return m;
  }
}
