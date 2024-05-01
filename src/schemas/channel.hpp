#pragma once

#include <chrono>
#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "../constants.hpp"
#include "../utils/chrono.hpp"

namespace bot::schemas {
  class Channel {
    public:
      Channel(const pqxx::row &row) {
        this->id = row[0].as<int>();
        this->alias_id = row[1].as<int>();
        this->alias_name = row[2].as<std::string>();

        this->joined_at =
            utils::chrono::string_to_time_point(row[3].as<std::string>());

        if (!row[4].is_null()) {
          this->opted_out_at =
              utils::chrono::string_to_time_point(row[4].as<std::string>());
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

    private:
      int id, alias_id;
      std::string alias_name;
      std::chrono::system_clock::time_point joined_at;
      std::optional<std::chrono::system_clock::time_point> opted_out_at;
  };

  class ChannelPreferences {
    public:
      ChannelPreferences(const pqxx::row &row) {
        this->channel_id = row[0].as<int>();

        if (!row[2].is_null()) {
          this->prefix = row[1].as<std::string>();
        } else {
          this->prefix = DEFAULT_PREFIX;
        }

        if (!row[3].is_null()) {
          this->locale = row[2].as<std::string>();
        } else {
          this->locale = DEFAULT_LOCALE_ID;
        }
      }

      ~ChannelPreferences() = default;

      const int &get_channel_id() const { return this->channel_id; }
      const std::string &get_prefix() const { return this->prefix; }
      const std::string &get_locale() const { return this->locale; }

    private:
      int channel_id;
      std::string prefix, locale;
  };
}
