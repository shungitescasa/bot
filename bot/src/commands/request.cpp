#include "commands/request.hpp"

#include <sol/types.hpp>

namespace bot::command {
  sol::table Request::as_lua_table(std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    o["command_id"] = this->command_id;
    if (this->subcommand_id.has_value()) {
      o["subcommand_id"] = this->subcommand_id.value();
    } else {
      o["subcommand_id"] = sol::lua_nil;
    }
    if (this->message.has_value()) {
      o["message"] = this->message.value();
    } else {
      o["message"] = sol::lua_nil;
    }

    o["sender"] = this->user.as_lua_table(luaState);
    o["channel"] = this->channel.as_lua_table(luaState);
    o["channel_preference"] = this->channel_preferences.as_lua_table(luaState);
    o["rights"] = this->user_rights.as_lua_table(luaState);

    return o;
  }
}