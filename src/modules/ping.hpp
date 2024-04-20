#pragma once

#include <string>
#include <variant>
#include <vector>

#include "../commands/command.hpp"

namespace bot {
  namespace mod {
    class Ping : public command::Command {
        std::string get_name() override { return "ping"; }

        std::variant<std::vector<std::string>, std::string> run(
            const irc::Message<irc::MessageType::Privmsg> &msg) override {
          return "pong";
        }
    };
  }
}
