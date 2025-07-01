#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <vector>

#include "../constants.hpp"
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

  enum ChannelFeature {
    MARKOV_RESPONSES,
    RANDOM_MARKOV_RESPONSES,
    SILENT_MODE
  };
  const std::vector<ChannelFeature> FEATURES = {
      MARKOV_RESPONSES, RANDOM_MARKOV_RESPONSES, SILENT_MODE};
  std::optional<ChannelFeature> string_to_channel_feature(
      const std::string &value);
  std::optional<std::string> channelfeature_to_string(
      const ChannelFeature &value);

  class ChannelPreferences {
    public:
      ChannelPreferences(const db::DatabaseRow &row) {
        this->channel_id = std::stoi(row.at("id"));
        this->prefix =
            row.at("prefix").empty() ? DEFAULT_PREFIX : row.at("prefix");
        this->locale =
            row.at("locale").empty() ? DEFAULT_LOCALE_ID : row.at("locale");

        std::for_each(
            FEATURES.begin(), FEATURES.end(),
            [this, &row](const ChannelFeature &f) {
              std::optional<std::string> feature = channelfeature_to_string(f);
              if (feature.has_value() && row.find(*feature) != row.end() &&
                  row.at(*feature) == "1") {
                this->features.push_back(f);
              }
            });
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
