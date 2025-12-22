#include "channel.hpp"

#include <optional>

namespace bot::schemas {
  sol::table Channel::as_lua_table(std::shared_ptr<sol::state> luaState) const {
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

  sol::table ChannelPreferences::as_lua_table(
      std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    o["id"] = this->channel_id;  // TODO: remove it later too.
    o["channel_id"] = this->channel_id;
    o["prefix"] = this->prefix;
    o["language"] = this->locale;
    o["is_silent"] = this->silent_mode;

    return o;
  }
}