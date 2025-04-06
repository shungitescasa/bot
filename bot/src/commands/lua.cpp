#include "commands/lua.hpp"

#include <sol/table.hpp>
#include <string>

#include "bundle.hpp"
#include "commands/request.hpp"
#include "commands/response.hpp"
#include "schemas/user.hpp"
#include "utils/chrono.hpp"
#include "utils/string.hpp"

namespace bot::command::lua {
  namespace library {
    void add_bot_library(std::shared_ptr<sol::state> state) {
      state->set_function("bot_get_compiler_version", []() {
        std::string info;
#ifdef __cplusplus
        info.append("C++" + std::to_string(__cplusplus).substr(2, 2));
#endif
#ifdef __VERSION__
        info.append(" (gcc " +
                    bot::utils::string::split_text(__VERSION__, ' ')[0] + ")");
#endif
        return info;
      });

      state->set_function("bot_get_uptime", []() {
        auto now = std::chrono::steady_clock::now();
        auto duration = now - START_TIME;
        auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return static_cast<long long>(seconds);
      });

      state->set_function("bot_get_memory_usage", []() {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss;
      });

      state->set_function("bot_get_compile_time",
                          []() { return BOT_COMPILED_TIMESTAMP; });

      state->set_function("bot_get_version", []() { return BOT_VERSION; });
    }

    void add_time_library(std::shared_ptr<sol::state> state) {
      state->set_function("time_current", []() {
        return static_cast<long long>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
      });

      state->set_function("time_humanize", [](const int &timestamp) {
        return utils::chrono::format_timestamp(timestamp);
      });
    }
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
    } else {
      this->level = schemas::PermissionLevel::USER;
    }

    this->handle = data["handle"];
  }

  Response LuaCommand::run(const InstanceBundle &bundle,
                           const Request &request) const {
    sol::object response = this->handle(request.as_lua_table(this->luaState));

    if (response.is<std::string>()) {
      return {response.as<std::string>()};
    } else if (response.is<sol::table>()) {
      sol::table tbl = response.as<sol::table>();
      std::vector<std::string> items;

      for (auto &kv : tbl) {
        sol::object value = kv.second;
        if (value.is<std::string>()) {
          items.push_back(value.as<std::string>());
        }
      }

      return items;
    }

    return {};
  }
}