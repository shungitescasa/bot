#pragma once

#include <chrono>
#include <optional>
#include <sol/sol.hpp>
#include <string>

#include "../utils/chrono.hpp"
#include "database.hpp"

namespace bot::schemas {
  class Channel {
    public:
      Channel(const db::DatabaseRow &row) {
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

      ~Channel() = default;

      const int &get_id() const { return this->id; }
      const int &get_alias_id() const { return this->alias_id; }
      const std::string &get_alias_name() const { return this->alias_name; }
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

  class ChannelPreferences {
    public:
      ChannelPreferences(const db::DatabaseRow &row) {
        this->channel_id = std::stoi(row.at("id"));
        this->prefix =
            row.at("prefix").empty() ? DEFAULT_PREFIX : row.at("prefix");
        this->locale =
            row.at("locale").empty() ? DEFAULT_LOCALE_ID : row.at("locale");
        this->silent_mode = std::stoi(row.at("silent_mode"));
      }

      ~ChannelPreferences() = default;

      const int &get_channel_id() const { return this->channel_id; }
      const std::string &get_prefix() const { return this->prefix; }
      const std::string &get_locale() const { return this->locale; }
      const bool &is_silent() const { return this->silent_mode; }

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;

    private:
      int channel_id;
      std::string prefix, locale;
      bool silent_mode;
  };
}
