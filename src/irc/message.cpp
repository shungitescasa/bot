#include "message.hpp"

#include <optional>
#include <string>
#include <vector>

namespace RedpilledBot {
  namespace IRC {
    std::optional<MessageType> define_message_type(const std::string &msg) {
      std::vector<std::string> parts = split_text(msg, ' ');
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
      }

      return std::nullopt;
    }
  }
}
