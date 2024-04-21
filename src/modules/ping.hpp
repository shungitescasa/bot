#pragma once

#include <string>
#include <variant>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"

namespace bot {
  namespace mod {
    class Ping : public command::Command {
        std::string get_name() const override { return "ping"; }

        std::variant<std::vector<std::string>, std::string> run(
            const InstanceBundle &bundle,
            const command::Request &request) const override {
          return "pong";
        }
    };
  }
}
