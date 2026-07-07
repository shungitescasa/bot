#pragma once

#include <string>

#include "core/command.hpp"

namespace bot::builtin {
  class PingCommand : public Command {
    public:
      const std::string get_name() const override { return "ping"; }

      const Response run(const Request &request) const override {
        return Response{"Pong!"};
      }
  };
}