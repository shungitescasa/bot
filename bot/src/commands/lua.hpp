#pragma once

#include <memory>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <string>
#include <vector>

#include "commands/command.hpp"
#include "commands/response.hpp"
#include "schemas/user.hpp"

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
    void add_net_library(std::shared_ptr<sol::state> state);
    void add_irc_library(std::shared_ptr<sol::state> state,
                         const InstanceBundle &bundle);

    void add_base_libraries(std::shared_ptr<sol::state> state);
  }

  command::Response run_safe_lua_script(const Request &request,
                                        const InstanceBundle &bundle,
                                        const std::string &script);

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

    private:
      std::string name;
      int delay;
      schemas::PermissionLevel level;
      std::vector<std::string> subcommands;

      sol::function handle;

      std::shared_ptr<sol::state> luaState;
  };
}