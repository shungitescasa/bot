#pragma once

#include <string>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/lua.hpp"
#include "commands/response_error.hpp"

namespace bot::mod {
  class LuaExecution : public command::Command {
      std::string get_name() const override { return "lua"; }

      int get_delay_seconds() const override { return 1; }

      command::Response run(const InstanceBundle &bundle,
                            const command::Request &request) const override {
        if (!request.message.has_value()) {
          throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
              request, bundle.localization, command::CommandArgument::VALUE);
        }

        std::string script = request.message.value();

        return command::lua::run_safe_lua_script(request, bundle, script);
      }
  };
}