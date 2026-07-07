#include "scriptvm/lua.hpp"

#include <memory>

#include "core/command.hpp"

namespace scriptvm::lua {
  bot::Response parse_lua_response(const sol::table &r, sol::object &res,
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

  LuaScriptLoader::LuaScriptLoader() {
    this->lua = std::make_shared<sol::state>();
    this->lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table,
                              sol::lib::math);
  }

  LuaCommand::LuaCommand(std::shared_ptr<sol::state> state,
                         const std::string &contents)
      : bot::Command("temp") {
    this->state = state;

    sol::table data = state->script(contents);
    name = data["name"];
    delay_seconds = data["delay_sec"];

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

    this->handle = data["handle"];
  }

  const bot::Response LuaCommand::run(const bot::Request &request) const {
    sol::table r;
    sol::object res = this->handle(r);
    return parse_lua_response(r, res, false);
  }
}