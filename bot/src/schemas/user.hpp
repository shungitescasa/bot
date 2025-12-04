#pragma once

#include <chrono>
#include <optional>
#include <sol/sol.hpp>
#include <string>

#include "../utils/chrono.hpp"
#include "database.hpp"

namespace bot::schemas {
  class User {
    public:
      User(const db::DatabaseRow &row) {
        this->id = std::stoi(row.at("id"));
        this->alias_id = std::stoi(row.at("alias_id"));
        this->alias_name = row.at("alias_name");

        this->joined_at =
            utils::chrono::string_to_time_point(row.at("joined_at"));

        if (!row.at("opted_out_at").empty()) {
          this->opted_out_at =
              utils::chrono::string_to_time_point(row.at("opted_out_at"));
        }
      }

      ~User() = default;

      const int &get_id() const { return this->id; }
      const int &get_alias_id() const { return this->alias_id; }
      const std::string &get_alias_name() const { return this->alias_name; }
      void set_alias_name(const std::string &alias_name) {
        this->alias_name = alias_name;
      }
      const std::chrono::system_clock::time_point &get_joined_at() const {
        return this->joined_at;
      }
      const std::optional<std::chrono::system_clock::time_point> &
      get_opted_out_at() const {
        return this->opted_out_at;
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;

    private:
      int id, alias_id;
      std::string alias_name;
      std::chrono::system_clock::time_point joined_at;
      std::optional<std::chrono::system_clock::time_point> opted_out_at;
  };

  enum PermissionLevel {
    SUSPENDED,
    USER,
    VIP,
    MODERATOR,
    BROADCASTER,
    TRUSTED = 50,
    SUPERUSER = 99
  };

  class UserRights {
    public:
      UserRights(const db::DatabaseRow &row) {
        this->id = std::stoi(row.at("id"));
        this->user_id = std::stoi(row.at("user_id"));
        this->channel_id = std::stoi(row.at("channel_id"));
        this->level = static_cast<PermissionLevel>(std::stoi(row.at("level")));
      }

      ~UserRights() = default;

      const int &get_id() const { return this->id; }
      const int &get_user_id() const { return this->user_id; }
      const int &get_channel_id() const { return this->channel_id; }
      const PermissionLevel &get_level() const { return this->level; }
      void set_level(PermissionLevel level) { this->level = level; }

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;

    private:
      int id, user_id, channel_id;
      PermissionLevel level;
  };
}
