#include "commands/lua.hpp"

#include <fmt/core.h>
#include <fmt/std.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <sol/sol.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "api/kick.hpp"
#include "api/twitch/helix_client.hpp"
#include "api/twitch/schemas/user.hpp"
#include "bundle.hpp"
#include "commands/request.hpp"
#include "commands/response.hpp"
#include "commands/response_error.hpp"
#include "config.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/multipart.h"
#include "cpr/response.h"
#include "database.hpp"
#include "rss.hpp"
#include "schemas/channel.hpp"
#include "schemas/stream.hpp"
#include "schemas/user.hpp"
#include "utils/chrono.hpp"
#include "utils/string.hpp"

namespace bot::command::lua {
  namespace library {

    void add_base_libraries(std::shared_ptr<sol::state> state) {
      add_bot_library(state);
      add_time_library(state);
      add_json_library(state);
      add_net_library(state);
      add_string_library(state);
      add_array_library(state);
      add_rss_library(state);
    }

    void add_chat_libraries(std::shared_ptr<sol::state> state,
                            const Request &request,
                            const InstanceBundle &bundle) {
      lua::library::add_bot_library(state, bundle);
      lua::library::add_irc_library(state, bundle);
      lua::library::add_twitch_library(state, request, bundle);
      lua::library::add_kick_library(state, bundle);
      lua::library::add_db_library(state);
      lua::library::add_l10n_library(state, bundle);
      lua::library::add_emote_library(state, bundle);
    }

    Response parse_lua_response(const sol::table &r, sol::object &res,
                                bool moon_prefix) {
      const std::string prefix = moon_prefix ? "🌑 " : "";

      if (res.get_type() == sol::type::function) {
        sol::function f = res.as<sol::function>();
        sol::object o = f(r);
        return parse_lua_response(r, o, moon_prefix);
      } else if (res.get_type() == sol::type::string) {
        return {prefix + res.as<std::string>()};
      } else if (res.get_type() == sol::type::number) {
        return {prefix + std::to_string(res.as<double>())};
      } else if (res.get_type() == sol::type::boolean) {
        return {prefix + std::to_string(res.as<bool>())};
      } else if (res.get_type() == sol::type::table) {
        sol::table t = res.as<sol::table>();
        std::vector<std::string> o;
        for (auto &kv : t) {
          if (kv.second.is<std::string>()) {
            o.push_back(prefix + kv.second.as<std::string>());
          }
        }
        return {o};
      } else if (res.get_type() == sol::type::lua_nil) {
        return {};
      } else {
        // should it be ResponseException?
        return {prefix + "Empty or unsupported response"};
      }
    }

    command::Response run_safe_lua_script(const Request &request,
                                          const InstanceBundle &bundle,
                                          const std::string &script,
                                          std::string lua_id,
                                          bool moon_prefix) {
      // shared_ptr is unnecessary here, but my library needs it.
      std::shared_ptr<sol::state> state = std::make_shared<sol::state>();

      state->open_libraries(sol::lib::base, sol::lib::table, sol::lib::string,
                            sol::lib::math);
      library::add_base_libraries(state);

      if (!lua_id.empty()) {
        library::add_storage_library(state, request, lua_id);
      }

      sol::load_result s = state->load("return " + script);
      if (!s.valid()) {
        s = state->load(script);
      }

      if (!s.valid()) {
        sol::error err = s;
        throw ResponseException<ResponseError::LUA_EXECUTION_ERROR>(
            request, bundle.localization, std::string(err.what()));
      }

      sol::protected_function_result res = s();

      if (!res.valid()) {
        sol::error err = s;
        throw ResponseException<ResponseError::LUA_EXECUTION_ERROR>(
            request, bundle.localization, std::string(err.what()));
      }

      sol::object o = res;

      return parse_lua_response(request.as_lua_table(state), o, moon_prefix);
    }

    LuaCommand::LuaCommand(std::shared_ptr<sol::state> luaState,
                           const std::string &script) {
      this->luaState = luaState;

      sol::table data = luaState->script(script);
      this->name = data["name"];
      this->delay = data["delay_sec"];

      sol::table subcommands = data["subcommands"];
      for (auto &k : subcommands) {
        sol::object value = k.second;
        if (value.is<std::string>()) {
          this->subcommands.push_back(value.as<std::string>());
        }
      }

      sol::table aliases = data["aliases"];
      for (auto &k : aliases) {
        sol::object value = k.second;
        if (value.is<std::string>()) {
          this->aliases.push_back(value.as<std::string>());
        }
      }

      std::string rights_text = data["minimal_rights"];
      if (rights_text == "suspended") {
        this->level = schemas::PermissionLevel::SUSPENDED;
      } else if (rights_text == "user") {
        this->level = schemas::PermissionLevel::USER;
      } else if (rights_text == "vip") {
        this->level = schemas::PermissionLevel::VIP;
      } else if (rights_text == "moderator") {
        this->level = schemas::PermissionLevel::MODERATOR;
      } else if (rights_text == "broadcaster") {
        this->level = schemas::PermissionLevel::BROADCASTER;
      } else if (rights_text == "trusted") {
        this->level = schemas::PermissionLevel::TRUSTED;
      } else if (rights_text == "superuser") {
        this->level = schemas::PermissionLevel::SUPERUSER;
      } else {
        this->level = schemas::PermissionLevel::USER;
      }

      this->handle = data["handle"];
    }

    Response LuaCommand::run(const InstanceBundle &bundle,
                             const Request &request) const {
      sol::table r = request.as_lua_table(this->luaState);
      sol::object response = this->handle(r);
      return parse_lua_response(r, response, false);
    }
  }