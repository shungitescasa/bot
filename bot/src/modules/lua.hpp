#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/lua.hpp"
#include "commands/response_error.hpp"
#include "cpr/api.h"
#include "utils/string.hpp"

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

  class LuaRemoteExecution : public command::Command {
      std::string get_name() const override { return "luaimport"; }

      int get_delay_seconds() const override { return 2; }

      command::Response run(const InstanceBundle &bundle,
                            const command::Request &request) const override {
        if (!request.message.has_value()) {
          throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
              request, bundle.localization, command::CommandArgument::VALUE);
        }

        std::string url = request.message.value();

        std::vector<std::string> mimeTypes = {"text/plain", "text/x-lua"};

        cpr::Response response = cpr::Get(
            cpr::Url{url},
            cpr::Header{{"Accept", utils::string::join_vector(mimeTypes, ',')},
                        {"User-Agent", "https://github.com/ilotterytea/bot"}});

        if (response.status_code != 200) {
          throw ResponseException<ResponseError::EXTERNAL_API_ERROR>(
              request, bundle.localization, response.status_code);
        }

        std::string contentType = response.header["Content-Type"];
        if (!std::any_of(
                mimeTypes.begin(), mimeTypes.end(),
                [&contentType](const auto &x) { return x == contentType; })) {
          throw ResponseException<ResponseError::INCORRECT_ARGUMENT>(
              request, bundle.localization, url);
        }

        std::string script = response.text;

        return command::lua::run_safe_lua_script(request, bundle, script);
      }
  };
}