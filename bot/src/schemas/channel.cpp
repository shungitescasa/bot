#include "channel.hpp"

#include <optional>

namespace bot::schemas {
  std::optional<ChannelFeature> string_to_channel_feature(
      const std::string &value) {
    if (value == "markov_responses") {
      return MARKOV_RESPONSES;
    } else if (value == "random_markov_responses") {
      return RANDOM_MARKOV_RESPONSES;
    } else if (value == "silent_mode") {
      return SILENT_MODE;
    } else {
      return std::nullopt;
    }
  }

  std::optional<std::string> channelfeature_to_string(
      const ChannelFeature &value) {
    switch (value) {
      case MARKOV_RESPONSES:
        return "markov_responses";
      case RANDOM_MARKOV_RESPONSES:
        return "random_markov_responses";
      case SILENT_MODE:
        return "silent_mode";
      default:
        std::nullopt;
    }
  }

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

    sol::table f = luaState->create_table();

    for (const ChannelFeature &feature : this->features) {
      std::optional<std::string> ff = channelfeature_to_string(feature);
      if (ff.has_value()) {
        f.add(ff.value());
      }
    }

    o["features"] = f;

    return o;
  }
}