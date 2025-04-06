#pragma once

#include <chrono>
#include <optional>
#include <pqxx/pqxx>
#include <sol/sol.hpp>
#include <string>
#include <vector>

#include "../constants.hpp"
#include "../utils/chrono.hpp"
#include "../utils/string.hpp"

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

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;

    private:
      int id, alias_id;
      std::string alias_name;
      std::chrono::system_clock::time_point joined_at;
      std::optional<std::chrono::system_clock::time_point> opted_out_at;
  };

  enum ChannelFeature { MARKOV_RESPONSES, RANDOM_MARKOV_RESPONSES };
  std::optional<ChannelFeature> string_to_channel_feature(
      const std::string &value);
  std::optional<std::string> channelfeature_to_string(
      const ChannelFeature &value);

  class ChannelPreferences {
    public:
      ChannelPreferences(const pqxx::row &row) {
        this->channel_id = row[0].as<int>();

        if (!row[1].is_null()) {
          this->prefix = row[1].as<std::string>();
        } else {
          this->prefix = DEFAULT_PREFIX;
        }

        if (!row[2].is_null()) {
          this->locale = row[2].as<std::string>();
        } else {
          this->locale = DEFAULT_LOCALE_ID;
        }

        if (!row[3].is_null()) {
          std::string x = row[3].as<std::string>();
          x = x.substr(1, x.length() - 2);
          std::vector<std::string> split_text =
              utils::string::split_text(x, ',');

          for (const std::string &part : split_text) {
            ChannelFeature feature = (ChannelFeature)std::stoi(part);
            this->features.push_back(feature);
          }
        }
      }

      ~ChannelPreferences() = default;

      const int &get_channel_id() const { return this->channel_id; }
      const std::string &get_prefix() const { return this->prefix; }
      const std::string &get_locale() const { return this->locale; }
      const std::vector<ChannelFeature> &get_features() const {
        return this->features;
      }

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;

    private:
      int channel_id;
      std::string prefix, locale;
      std::vector<ChannelFeature> features;
  };
}
