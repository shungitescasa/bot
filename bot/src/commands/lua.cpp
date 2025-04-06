#include "commands/lua.hpp"

#include <sol/table.hpp>
#include <string>

#include "bundle.hpp"
#include "commands/request.hpp"
#include "commands/response.hpp"
#include "schemas/user.hpp"

namespace bot::command::lua {
  LuaCommand::LuaCommand(std::shared_ptr<sol::state> luaState,
                         const std::string &script) {
    this->luaState = luaState;

    sol::table data = luaState->script(script);
    this->name = data["name"];
    this->delay = data["delay_sec"];

    sol::table subcommands = data["subcommands"];
    for (auto &k : subcommands) {
      sol::object value = k.second;
      if (value.is<std::string>()) {
        this->subcommands.push_back(value.as<std::string>());
      }
    }

    std::string rights_text = data["minimal_rights"];
    if (rights_text == "suspended") {
      this->level = schemas::PermissionLevel::SUSPENDED;
    } else if (rights_text == "user") {
      this->level = schemas::PermissionLevel::USER;
    } else if (rights_text == "vip") {
      this->level = schemas::PermissionLevel::VIP;
    } else if (rights_text == "moderator") {
      this->level = schemas::PermissionLevel::MODERATOR;
    } else if (rights_text == "broadcaster") {
      this->level = schemas::PermissionLevel::BROADCASTER;
    } else {
      this->level = schemas::PermissionLevel::USER;
    }

    this->handle = data["run"];
  }

  Response LuaCommand::run(const InstanceBundle &bundle,
                           const Request &request) const {
    sol::object response = this->handle(request.as_lua_table(this->luaState));

    if (response.is<std::string>()) {
      return {response.as<std::string>()};
    } else if (response.is<sol::table>()) {
      sol::table tbl = response.as<sol::table>();
      std::vector<std::string> items;

      for (auto &kv : tbl) {
        sol::object value = kv.second;
        if (value.is<std::string>()) {
          items.push_back(value.as<std::string>());
        }
      }

      return items;
    }

    return {};
  }
}