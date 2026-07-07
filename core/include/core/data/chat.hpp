#pragma once

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <chrono>
#include <memory>
#include <optional>
#include <sol/sol.hpp>
#include <string>

#include "core/data/database.hpp"
#include "core/utils.hpp"
#include "rpc/msgpack.hpp"

namespace bot::data {
  using Timestamp = long long;

  struct Room {
      Room() = default;
      ~Room() = default;

      Room(const DatabaseRow &row) {
        if (row.contains("name")) name = row.at("name");
        if (row.contains("id")) id = std::stoi(row.at("id"));

        if (row.contains("alias_id")) alias_id = std::stoi(row.at("alias_id"));

        if (row.contains("joined_at")) {
          joined_at = utils::chrono::string_to_time_point(row.at("joined_at"))
                          .time_since_epoch()
                          .count();
        }

        if (row.contains("parted_at") && !row.at("parted_at").empty()) {
          parted_at = utils::chrono::string_to_time_point(row.at("parted_at"))
                          .time_since_epoch()
                          .count();
        }
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const {
        sol::table o = state->create_table();

        if (id != -1) o["id"] = id;
        if (alias_id != -1) o["alias_id"] = alias_id;

        if (!name.empty()) {
          o["alias_name"] = name;  // backward compatibility
          o["name"] = name;
        }

        o["joined_at"] = joined_at;

        if (this->parted_at.has_value()) {
          o["opted_out_at"] = parted_at.value();  // backward compatibility
          o["parted_at"] = parted_at.value();
        } else {
          o["opted_out_at"] = sol::lua_nil;  // backward compatibility
          o["parted_at"] = sol::lua_nil;
        }

        return o;
      }

      std::string name = "";
      int id = -1, alias_id = -1;
      Timestamp joined_at;
      std::optional<Timestamp> parted_at;

      MSGPACK_DEFINE(name, id, alias_id, joined_at, parted_at);
  };

  struct RoomPreferences {
      RoomPreferences() = default;
      ~RoomPreferences() = default;

      RoomPreferences(const DatabaseRow &row) {
        if (row.contains("id")) room_id = std::stoi(row.at("id"));

        if (row.contains("prefix") && !row.at("prefix").empty())
          prefix = row.at("prefix");

        if (row.contains("locale") && !row.at("locale").empty())
          locale = row.at("locale");

        if (row.contains("silent_mode"))
          silent_mode = std::stoi(row.at("silent_mode"));
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const {
        sol::table o = state->create_table();

        o["id"] = room_id;          // backward compatibility
        o["channel_id"] = room_id;  // backward compatibility
        o["room_id"] = room_id;     // backward compatibility

        o["prefix"] = prefix;
        o["language"] = locale;
        o["is_silent"] = silent_mode;

        return o;
      }

      int room_id = -1;
      std::string prefix = "", locale = "";
      bool silent_mode = false;

      MSGPACK_DEFINE(room_id, prefix, locale, silent_mode);
  };

  struct Sender {
      Sender() = default;
      ~Sender() = default;

      Sender(const DatabaseRow &row) {
        if (row.contains("name")) name = row.at("name");
        if (row.contains("id")) id = std::stoi(row.at("id"));

        if (row.contains("alias_id")) alias_id = std::stoi(row.at("alias_id"));

        if (row.contains("joined_at")) {
          joined_at = utils::chrono::string_to_time_point(row.at("joined_at"))
                          .time_since_epoch()
                          .count();
        }

        if (row.contains("parted_at") && !row.at("parted_at").empty()) {
          parted_at = utils::chrono::string_to_time_point(row.at("parted_at"))
                          .time_since_epoch()
                          .count();
        }
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const {
        sol::table o = state->create_table();

        if (id != -1) o["id"] = id;
        if (alias_id != -1) o["alias_id"] = alias_id;

        if (!name.empty()) {
          o["alias_name"] = name;  // backward compatibility
          o["name"] = name;
        }

        o["joined_at"] = joined_at;

        if (this->parted_at.has_value()) {
          o["opted_out_at"] = parted_at.value();  // backward compatibility
          o["parted_at"] = parted_at.value();
        } else {
          o["opted_out_at"] = sol::lua_nil;  // backward compatibility
          o["parted_at"] = sol::lua_nil;
        }

        return o;
      }

      std::string name = "";
      int id = -1, alias_id = -1;
      Timestamp joined_at;
      std::optional<Timestamp> parted_at;

      MSGPACK_DEFINE(name, id, alias_id, joined_at, parted_at);
  };

  enum class PermissionLevel {
    Suspended = 0,
    User,
    VIP,
    Moderator,
    Broadcaster,
    Trusted = 50,
    Superuser = 99
  };

  struct SenderRights {
      SenderRights() = default;
      ~SenderRights() = default;

      SenderRights(const DatabaseRow &row) {
        if (row.contains("id")) id = std::stoi(row.at("id"));
        if (row.contains("sender_id"))
          sender_id = std::stoi(row.at("sender_id"));
        if (row.contains("room_id")) room_id = std::stoi(row.at("room_id"));

        if (row.contains("level")) level = std::stoi(row.at("level"));
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const {
        sol::table o = state->create_table();
        o["id"] = id;

        o["user_id"] = sender_id;  // backward compatibility
        o["sender_id"] = sender_id;
        o["channel_id"] = room_id;  // backward compatibility
        o["room_id"] = room_id;

        o["level"] = level;
        o["is_fixed"] = false;  // backward compatibility
        return o;
      }

      int id = -1, sender_id = -1, room_id = -1,
          level = static_cast<int>(PermissionLevel::User);

      MSGPACK_DEFINE(id, sender_id, room_id, level);
  };
}