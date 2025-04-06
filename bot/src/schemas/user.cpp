#include "schemas/user.hpp"

#include <chrono>
#include <sol/types.hpp>

namespace bot::schemas {
  sol::table User::as_lua_table(std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    o["id"] = this->id;
    o["alias_id"] = this->alias_id;
    o["alias_name"] = this->alias_name;

    o["joined_at"] =
        static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
                              this->joined_at.time_since_epoch())
                              .count());
    if (this->opted_out_at.has_value()) {
      o["opted_out_at"] =
          static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
                                this->opted_out_at->time_since_epoch())
                                .count());
    } else {
      o["opted_out_at"] = sol::lua_nil;
    }

    return o;
  }

  sol::table UserRights::as_lua_table(
      std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    o["id"] = this->id;
    o["user_id"] = this->user_id;
    o["channel_id"] = this->channel_id;
    o["level"] = this->level;
    o["is_fixed"] = false;  // TODO: remove it later

    return o;
  }
}