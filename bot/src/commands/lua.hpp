#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <string>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/response.hpp"
#include "commands/response_error.hpp"
#include "config.hpp"
#include "cpr/api.h"
#include "schemas/user.hpp"
#include "utils/string.hpp"

void print_lua_object_type(const sol::object &obj);

namespace bot::command::lua {
  namespace library {
    void add_bot_library(std::shared_ptr<sol::state> state);
    void add_bot_library(std::shared_ptr<sol::state> state,
                         const InstanceBundle &bundle);
    void add_time_library(std::shared_ptr<sol::state> state);
    void add_json_library(std::shared_ptr<sol::state> state);
    void add_twitch_library(std::shared_ptr<sol::state> state,
                            const Request &request,
                            const InstanceBundle &bundle);
    void add_kick_library(std::shared_ptr<sol::state> state,
                          const InstanceBundle &bundle);
    void add_net_library(std::shared_ptr<sol::state> state);
    void add_db_library(std::shared_ptr<sol::state> state,
                        const Configuration &config);
    void add_irc_library(std::shared_ptr<sol::state> state,
                         const InstanceBundle &bundle);
    void add_l10n_library(std::shared_ptr<sol::state> state,
                          const InstanceBundle &bundle);
    void add_storage_library(std::shared_ptr<sol::state> state,
                             const Request &request, const Configuration &cfg,
                             const std::string &lua_id);

    void add_rss_library(std::shared_ptr<sol::state> state);

    void add_base_libraries(std::shared_ptr<sol::state> state);
    void add_chat_libraries(std::shared_ptr<sol::state> state,
                            const Request &request,
                            const InstanceBundle &bundle);
  }

  command::Response run_safe_lua_script(const Request &request,
                                        const InstanceBundle &bundle,
                                        const std::string &script,
                                        std::string lua_id = "");

  class LuaCommand : public Command {
    public:
      LuaCommand(std::shared_ptr<sol::state> luaState,
                 const std::string &content);
      ~LuaCommand() = default;

      Response run(const InstanceBundle &bundle,
                   const Request &request) const override;

      std::string get_name() const override { return this->name; }
      int get_delay_seconds() const override { return this->delay; }
      schemas::PermissionLevel get_permission_level() const override {
        return this->level;
      }
      std::vector<std::string> get_subcommand_ids() const override {
        return this->subcommands;
      }
      std::vector<std::string> get_aliases() const override {
        return this->aliases;
      }

    private:
      std::string name;
      int delay;
      schemas::PermissionLevel level;
      std::vector<std::string> subcommands, aliases;

      sol::function handle;

      std::shared_ptr<sol::state> luaState;
  };

  namespace mod {
    class LuaExecution : public command::Command {
        std::string get_name() const override { return "lua"; }

        int get_delay_seconds() const override { return 0; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!bundle.configuration.lua.allow_arbitrary_scripts &&
              request.requester.user_rights.get_level() < schemas::TRUSTED) {
            throw ResponseException<ResponseError::ILLEGAL_COMMAND>(
                request, bundle.localization);
          }

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

        int get_delay_seconds() const override { return 0; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::CommandArgument::VALUE);
          }

          if (!bundle.configuration.lua.allow_arbitrary_scripts &&
              request.requester.user_rights.get_level() < schemas::TRUSTED &&
              !std::any_of(bundle.configuration.lua.script_whitelist.begin(),
                           bundle.configuration.lua.script_whitelist.end(),
                           [&request](const std::string &i) {
                             return i == request.message.value();
                           })) {
            throw ResponseException<ResponseError::ILLEGAL_COMMAND>(
                request, bundle.localization);
          }

          std::vector<std::string> parts =
              utils::string::split_text(request.message.value(), ' ');

          std::string url = parts[0];

          parts.erase(parts.begin());

          std::string m = utils::string::join_vector(parts, ' ');

          command::Request r = request;

          if (m.empty()) {
            r.message = std::nullopt;
          } else {
            r.message = m;
          }

          std::vector<std::string> mimeTypes = {"text/plain", "text/x-lua",
                                                "text/plain; charset=utf-8",
                                                "text/x-lua; charset=utf-8"};

          cpr::Response response = cpr::Get(
              cpr::Url{url},
              cpr::Header{
                  {"Accept", utils::string::join_vector(mimeTypes, ',')},
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

          return command::lua::run_safe_lua_script(r, bundle, script, url);
        }
    };
  }
}