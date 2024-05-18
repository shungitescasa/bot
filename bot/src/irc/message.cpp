#include "message.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bot {
  namespace irc {
    std::optional<MessageType> define_message_type(const std::string &msg) {
      std::vector<std::string> parts = utils::string::split_text(msg, ' ');
      int i;

      if (msg[0] == '@') {
        i = 2;
      } else if (msg[0] == ':') {
        i = 1;
      } else {
        return std::nullopt;
      }

      if (parts[i] == "NOTICE") {
        return MessageType::Notice;
      } else if (parts[i] == "PRIVMSG") {
        return MessageType::Privmsg;
      } else if (parts[i] == "PING") {
        return MessageType::Ping;
      }

      return std::nullopt;
    }
  }
}
