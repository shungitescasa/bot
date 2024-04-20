#include <string>
#include <variant>
#include <vector>

#include "../irc/message.hpp"

namespace bot {
  namespace command {
    class Command {
        virtual std::string get_name() = 0;
        virtual std::variant<std::vector<std::string>, std::string> run(
            const irc::Message<irc::MessageType::Privmsg> &msg) = 0;
    };
  }
}
