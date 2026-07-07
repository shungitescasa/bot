#pragma once

#include <string>

#include "core/command.hpp"

namespace bot::builtin {
  class PingCommand : public Command {
    public:
      PingCommand() : Command("ping2") {}

      const Response run(const Request &request) const override {
        return Response{"Pong!"};
      }
  };
}