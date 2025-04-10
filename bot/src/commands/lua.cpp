#include "commands/lua.hpp"

#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "api/twitch/schemas/user.hpp"
#include "bundle.hpp"
#include "commands/request.hpp"
#include "commands/response.hpp"
#include "commands/response_error.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/multipart.h"
#include "cpr/response.h"
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

      state->set_function("bot_config", [state]() {
        std::optional<bot::Configuration> o_cfg =
            bot::parse_configuration_from_file(".env");

        if (!o_cfg.has_value()) {
          return sol::make_object(*state, sol::nil);
        }

        return sol::make_object(*state, o_cfg->as_lua_table(state));
      });
    }

    void add_time_library(std::shared_ptr<sol::state> state) {
      state->set_function("time_current", []() {
        return static_cast<long long>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
      });

      state->set_function("time_humanize", [](const int &timestamp) {
        return utils::chrono::format_timestamp(timestamp);
      });

      state->set_function("time_format",
                          [](const long &timestamp, const std::string &format) {
                            std::time_t t = std::time(nullptr);
                            t = timestamp;
                            std::tm *now = std::localtime(&t);

                            std::ostringstream oss;
                            oss << std::put_time(now, format.c_str());

                            std::string o = oss.str();

                            return o;
                          });

      state->set_function("time_parse", [](const std::string &datetime,
                                           const std::string &format) {
        std::tm tm = {};
        std::istringstream iss(datetime);
        iss >> std::get_time(&tm, format.c_str());
        if (iss.fail()) {
          throw std::runtime_error("datetime parse error");
        }

        return static_cast<long long>(std::mktime(&tm));
      });
    }

    sol::object parse_json_object(std::shared_ptr<sol::state_view> state,
                                  nlohmann::json j) {
      switch (j.type()) {
        case nlohmann::json::value_t::null:
          return sol::make_object(*state, sol::lua_nil);
        case nlohmann::json::value_t::string:
          return sol::make_object(*state, j.get<std::string>());
        case nlohmann::json::value_t::number_integer:
          return sol::make_object(*state, j.get<int>());
        case nlohmann::json::value_t::number_unsigned:
          return sol::make_object(*state, j.get<unsigned int>());
        case nlohmann::json::value_t::number_float:
          return sol::make_object(*state, j.get<double>());
        case nlohmann::json::value_t::boolean:
          return sol::make_object(*state, j.get<bool>());
        case nlohmann::json::value_t::array: {
          sol::table a = state->create_table();

          for (int i = 0; i < j.size(); ++i) {
            a[i] = parse_json_object(state, j[i]);
          }

          return sol::make_object(*state, a);
        }
        case nlohmann::json::value_t::object: {
          sol::table o = state->create_table();

          for (const auto &[k, v] : j.items()) {
            o[k] = parse_json_object(state, v);
          }

          return sol::make_object(*state, o);
        }
        default:
          throw std::runtime_error("Unsupported Lua type: " +
                                   std::string(j.type_name()));
      }
    }

    nlohmann::json lua_to_json(sol::object o) {
      switch (o.get_type()) {
        case sol::type::lua_nil:
          return nullptr;
        case sol::type::string:
          return o.as<std::string>();
        case sol::type::boolean:
          return o.as<bool>();
        case sol::type::number: {
          double num = o.as<double>();
          if (std::floor(num) == num) {
            return static_cast<long long>(num);
          }
          return num;
        }
        case sol::type::table: {
          sol::table t = o;

          bool is_array = true;
          int count = 0;
          for (auto &kv : t) {
            sol::object key = kv.first;
            if (key.get_type() != sol::type::number) {
              is_array = false;
              break;
            }
            ++count;
          }

          if (is_array) {
            nlohmann::json a = nlohmann::json::array();
            for (size_t i = 1; i <= count; ++i) {
              a.push_back(lua_to_json(t[i]));
            }
            return a;
          } else {
            nlohmann::json ob = nlohmann::json::object();
            for (auto &kv : t) {
              std::string key = kv.first.as<std::string>();
              ob[key] = lua_to_json(kv.second);
            }
            return ob;
          }
        }
        default:
          throw std::runtime_error(
              "Unsupported Lua object for JSON conversion");
      }
    }

    void add_json_library(std::shared_ptr<sol::state> state) {
      state->set_function("json_parse", [state](const std::string &s) {
        nlohmann::json j = nlohmann::json::parse(s);
        return parse_json_object(state, j);
      });

      state->set_function("json_stringify", [](const sol::object &o) {
        return lua_to_json(o).dump();
      });

      state->set_function("json_get_value", [state](const sol::object &body,
                                                    const std::string &path) {
        std::vector<std::string> parts = utils::string::split_text(path, '.');
        nlohmann::json o = lua_to_json(body);

        for (const std::string &path : parts) {
          o = o[path];
        }

        return parse_json_object(state, o);
      });
    }

    void add_net_library(std::shared_ptr<sol::state> state) {
      state->set_function("net_get", [state](const std::string &url) {
        sol::table t = state->create_table();

        cpr::Response response = cpr::Get(cpr::Url{url});

        t["code"] = response.status_code;
        t["text"] = response.text;

        return t;
      });

      state->set_function(
          "net_get_with_headers",
          [state](const std::string &url, const sol::table &headers) {
            sol::table t = state->create_table();

            cpr::Header h{};

            for (auto &kv : headers) {
              h[kv.first.as<std::string>()] = kv.second.as<std::string>();
            }

            cpr::Response response = cpr::Get(cpr::Url{url}, h);

            t["code"] = response.status_code;
            t["text"] = response.text;

            return t;
          });

      state->set_function(
          "net_post_multipart_with_headers",
          [state](const std::string &url, const sol::table &body,
                  const sol::table &headers) {
            sol::table t = state->create_table();

            cpr::Header h{};

            for (auto &kv : headers) {
              h[kv.first.as<std::string>()] = kv.second.as<std::string>();
            }

            cpr::Multipart multipart = {};
            for (auto &kv : body) {
              multipart.parts.push_back(
                  {kv.first.as<std::string>(), kv.second.as<std::string>()});
            }

            cpr::Response response = cpr::Post(cpr::Url{url}, multipart, h);

            t["code"] = response.status_code;
            t["text"] = response.text;

            return t;
          });
    }

    void add_l10n_library(std::shared_ptr<sol::state> state) {
      state->set_function(
          "l10n_custom_formatted_line_request",
          [](const sol::table &request, const sol::table &lines,
             const std::string &line_id, const sol::table &parameters) {
            // TODO: use Localization class instead!!!

            // TODO: convert the table to C++ struct for type safety later
            std::string language = request["channel_preference"]["language"];

            if (!lines[language].valid() || !lines[language][line_id].valid()) {
            }

            std::string line = lines[language][line_id];

            std::vector<std::string> args;

            for (auto &kv : parameters) {
              args.push_back(kv.second.as<std::string>());
            }

            int pos = 0;
            int index = 0;

            while ((pos = line.find("%s", pos)) != std::string::npos) {
              line.replace(pos, 2, args[index]);
              pos += args[index].size();
              ++index;

              if (index >= args.size()) {
                break;
              }
            }

            std::map<std::string, std::string> token_map = {
                {"{sender.alias_name}", request["sender"]["alias_name"]},
                {"{source.alias_name}", request["channel"]["alias_name"]},
                {"{default.prefix}", DEFAULT_PREFIX}};

            for (const auto &pair : token_map) {
              int pos = line.find(pair.first);

              while (pos != std::string::npos) {
                line.replace(pos, pair.first.length(), pair.second);
                pos = line.find(pair.first, pos + pair.second.length());
              }
            }

            return line;
          });
    }

    void add_base_libraries(std::shared_ptr<sol::state> state) {
      add_bot_library(state);
      add_time_library(state);
      add_json_library(state);
      add_net_library(state);
      add_l10n_library(state);
    }

    void add_twitch_library(std::shared_ptr<sol::state> state,
                            const Request &request,
                            const InstanceBundle &bundle) {
      // TODO: ratelimits
      state->set_function("twitch_get_chatters", [state, &request, &bundle]() {
        auto chatters = bundle.helix_client.get_chatters(
            request.channel.get_alias_id(), bundle.irc_client.get_bot_id());

        sol::table o = state->create_table();

        std::for_each(chatters.begin(), chatters.end(),
                      [state, &o](const api::twitch::schemas::User &x) {
                        sol::table u = state->create_table();

                        u["id"] = x.id;
                        u["login"] = x.login;

                        o.add(u);
                      });

        return o;
      });
    }
  }

  std::string parse_lua_response(const sol::table &r, sol::object &res) {
    if (res.get_type() == sol::type::function) {
      sol::function f = res.as<sol::function>();
      sol::object o = f(r);
      return parse_lua_response(r, o);
    } else if (res.get_type() == sol::type::string) {
      return {"🌑 " + res.as<std::string>()};
    } else if (res.get_type() == sol::type::number) {
      return {"🌑 " + std::to_string(res.as<double>())};
    } else if (res.get_type() == sol::type::boolean) {
      return {"🌑 " + std::to_string(res.as<bool>())};
    } else {
      // should it be ResponseException?
      return "Empty or unsupported response";
    }
  }

  command::Response run_safe_lua_script(const Request &request,
                                        const InstanceBundle &bundle,
                                        const std::string &script) {
    // shared_ptr is unnecessary here, but my library needs it.
    std::shared_ptr<sol::state> state = std::make_shared<sol::state>();

    state->open_libraries(sol::lib::base, sol::lib::table, sol::lib::string);
    library::add_base_libraries(state);

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

    return parse_lua_response(request.as_lua_table(state), o);
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
    sol::table r = request.as_lua_table(this->luaState);
    sol::object response = this->handle(r);
    return parse_lua_response(r, response);
  }
}