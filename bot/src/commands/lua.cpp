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

      state->set_function("bot_get_temperature", []() {
        float temp = 0.0;

        std::string path = "/sys/class/thermal/thermal_zone0/temp";

        if (!std::filesystem::exists(path)) {
          return temp;
        }

        std::ifstream ifs;
        ifs.open(path);

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ifs.close();

        temp = std::stof(buffer.str());
        temp /= 1000;
        temp = std::roundf(temp * 100) / 100;

        return temp;
      });

      state->set_function("bot_get_compile_time",
                          []() { return BOT_COMPILED_TIMESTAMP; });

      state->set_function("bot_get_version", []() { return BOT_VERSION; });

      state->set_function("bot_config", [state]() {
        std::optional<bot::Configuration> o_cfg =
            bot::parse_configuration_from_file(".env");

        if (!o_cfg.has_value()) {
          return sol::make_object(*state, sol::lua_nil);
        }

        return sol::make_object(*state, o_cfg->as_lua_table(state));
      });
    }

    void add_bot_library(std::shared_ptr<sol::state> state,
                         const InstanceBundle &bundle) {
      state->set_function("bot_username", [&bundle]() {
        return bundle.irc_client.get_username();
      });

      state->set_function("bot_get_loaded_command_names", [state, &bundle]() {
        sol::table o = state->create_table();

        const std::vector<std::unique_ptr<Command>> &commands =
            bundle.command_loader.get_commands();

        std::for_each(commands.begin(), commands.end(),
                      [&o](const std::unique_ptr<Command> &command) {
                        o.add(command->get_name());
                      });

        return o;
      });

      add_bot_library(state);
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

      state->set_function("time_humanize", [](const double &timestamp) {
        return utils::chrono::format_timestamp(std::floor(timestamp));
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
            a[i + 1] = parse_json_object(state, j[i]);
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
          "net_post", [state](const std::string &url, const sol::table &body) {
            sol::table t = state->create_table();

            cpr::Multipart multipart = {};
            for (auto &kv : body) {
              multipart.parts.push_back(
                  {kv.first.as<std::string>(), kv.second.as<std::string>()});
            }

            cpr::Response response = cpr::Post(cpr::Url{url}, multipart);

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

    void add_l10n_library(std::shared_ptr<sol::state> state,
                          const InstanceBundle &bundle) {
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
                {"{default.prefix}", DEFAULT_PREFIX},
                {"{channel.prefix}", request["channel_preference"]["prefix"]}};

            for (const auto &pair : token_map) {
              int pos = line.find(pair.first);

              while (pos != std::string::npos) {
                line.replace(pos, pair.first.length(), pair.second);
                pos = line.find(pair.first, pos + pair.second.length());
              }
            }

            return line;
          });

      state->set_function("l10n_get_localization_names", [state, &bundle]() {
        sol::table o = state->create_table();

        auto locales = bundle.localization.get_loaded_localizations();

        std::for_each(locales.begin(), locales.end(),
                      [&o](const std::string &x) { o.add(x); });

        return o;
      });
    }

    void add_string_library(std::shared_ptr<sol::state> state) {
      state->set_function("str_format",
                          [](const std::string &str, const sol::table &params) {
                            std::vector<std::string> p;
                            std::string s = str;
                            long pos = std::string::npos;
                            for (const auto &x : params) {
                              if (x.second.is<std::string>()) {
                                pos = s.find("{}");
                                if (pos == std::string::npos) {
                                  break;
                                }
                                s.replace(pos, 2, x.second.as<std::string>());
                              }
                            }
                            return s;
                          });

      state->set_function(
          "str_split", [state](const std::string &text, const char &delimiter) {
            sol::table o = state->create_table();
            std::vector<std::string> parts =
                utils::string::split_text(text, delimiter);

            std::for_each(parts.begin(), parts.end(),
                          [&o](const std::string &part) { o.add(part); });

            return o;
          });

      state->set_function("event_type_to_str", [](const int &v) {
        return schemas::event_type_to_string(v);
      });

      state->set_function("str_to_event_type", [](const std::string &v) {
        return (int)schemas::string_to_event_type(v);
      });

      state->set_function("str_to_event_flag", [state](const std::string &v) {
        auto o = schemas::string_to_event_flag(v);
        if (o.has_value()) {
          return sol::make_object(*state, o.value());
        } else {
          return sol::make_object(*state, sol::lua_nil);
        }
      });

      state->set_function("event_flag_to_str", [state](const int &v) {
        auto o = schemas::event_flag_to_string(v);
        if (o.has_value()) {
          return sol::make_object(*state, o.value());
        } else {
          return sol::make_object(*state, sol::lua_nil);
        }
      });

      state->set_function("str_make_parts", [state](
                                                const std::string &base,
                                                const sol::table &values,
                                                const std::string &prefix,
                                                const std::string &separator,
                                                const long long &max_length) {
        std::vector<std::string> lines = {""};
        int index = 0;

        for (auto &[_, v] : values) {
          const std::string &m = lines.at(index);
          std::string x = prefix + v.as<std::string>();

          if (base.length() + m.length() + x.length() + separator.length() >=
              max_length) {
            index += 1;
          }

          if (index > lines.size() - 1) {
            lines.push_back(x);
          } else {
            lines[index] = m + separator + x;
          }
        }

        sol::table o = state->create_table();
        std::for_each(lines.begin(), lines.end(),
                      [&o, &base](const std::string &x) { o.add(base + x); });

        return o;
      });
    }

    void add_db_library(std::shared_ptr<sol::state> state,
                        const Configuration &cfg) {
      state->set_function("db_execute", [state, cfg](
                                            const std::string &query,
                                            const sol::table &parameters) {
        std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);

        std::vector<std::string> params;

        for (const auto &kv : parameters) {
          auto v = kv.second;
          switch (v.get_type()) {
            case sol::type::lua_nil: {
              params.push_back("NULL");
              break;
            }
            case sol::type::string: {
              params.push_back(v.as<std::string>());
              break;
            }
            case sol::type::boolean: {
              params.push_back(std::to_string(v.as<bool>()));
              break;
            }
            case sol::type::number: {
              double num = v.as<double>();
              if (std::floor(num) == num) {
                params.push_back(std::to_string(static_cast<long long>(num)));
              } else {
                params.push_back(std::to_string(num));
              }
              break;
            }
            default:
              throw std::runtime_error("Unsupported Lua type for DB queries");
          }
        }

        conn->exec(query, params);
      });

      state->set_function("db_query", [state, cfg](
                                          const std::string &query,
                                          const sol::table &parameters) {
        std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);

        std::vector<std::string> params;

        for (const auto &kv : parameters) {
          auto v = kv.second;
          switch (v.get_type()) {
            case sol::type::lua_nil: {
              params.push_back("NULL");
              break;
            }
            case sol::type::string: {
              params.push_back(v.as<std::string>());
              break;
            }
            case sol::type::boolean: {
              params.push_back(std::to_string(v.as<bool>()));
              break;
            }
            case sol::type::number: {
              double num = v.as<double>();
              if (std::floor(num) == num) {
                params.push_back(std::to_string(static_cast<long long>(num)));
              } else {
                params.push_back(std::to_string(num));
              }
              break;
            }
            default:
              throw std::runtime_error("Unsupported Lua type for DB queries");
          }
        }

        db::DatabaseRows rows = conn->exec(query, params);

        sol::table o = state->create_table();

        for (const db::DatabaseRow &row : rows) {
          sol::table r = state->create_table();

          for (const auto &[k, v] : row) {
            sol::object val;
            if (v.empty()) {
              val = sol::make_object(*state, sol::lua_nil);
            } else {
              val = sol::make_object(*state, v);
            }
            r[k] = val;
          }

          o.add(r);
        }

        return o;
      });
    }

    void add_array_library(std::shared_ptr<sol::state> state) {
      state->set_function("array_contains_int", [](const sol::table &haystack,
                                                   const long long &needle) {
        bool o = false;
        for (auto &[_, v] : haystack) {
          if (v.is<long long>()) {
            o = v.as<long long>() == needle;
            if (o) break;
          }
        }
        return o;
      });

      state->set_function("array_contains", [](const sol::table &haystack,
                                               const std::string &needle) {
        bool o = false;
        for (auto &[_, v] : haystack) {
          if (v.is<std::string>()) {
            o = v.as<std::string>() == needle;
            if (o) break;
          }
        }
        return o;
      });
    }

    void add_rss_library(std::shared_ptr<sol::state> state) {
      state->set_function("rss_get", [state](const std::string &url) {
        std::optional<RSSChannel> channel = bot::get_rss_channel(url);
        if (!channel.has_value()) {
          return sol::make_object(*state, sol::lua_nil);
        }

        return sol::make_object(*state, channel->as_lua_table(state));
      });
    }

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
      lua::library::add_db_library(state, bundle.configuration);
      lua::library::add_l10n_library(state, bundle);
    }

    void add_irc_library(std::shared_ptr<sol::state> state,
                         const InstanceBundle &bundle) {
      state->set_function("irc_join_channel",
                          [&bundle](const std::string &channel_name) {
                            return bundle.irc_client.join(channel_name);
                          });

      state->set_function("irc_join_channel", [&bundle](const int &channel_id) {
        return bundle.irc_client.join(channel_id);
      });

      state->set_function("irc_send_message",
                          [&bundle](const std::string &channel_name,
                                    const std::string &message) {
                            bundle.irc_client.say(channel_name, message);
                          });

      state->set_function(
          "irc_send_message",
          [&bundle](const int &channel_id, const std::string &message) {
            bundle.irc_client.say(channel_id, message);
          });
    }

    void add_twitch_library(std::shared_ptr<sol::state> state,
                            const Request &request,
                            const InstanceBundle &bundle) {
      // TODO: ratelimits
      state->set_function("twitch_get_chatters", [state, &request, &bundle]() {
        auto chatters = bundle.helix_client.get_chatters(
            request.requester.channel.get_alias_id(),
            bundle.irc_client.get_user_id());

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

      state->set_function(
          "twitch_get_users", [state, &bundle](const sol::table &names) {
            std::vector<int> ids;
            std::vector<std::string> logins;

            for (auto &[k, v] : names) {
              if (!v.is<sol::table>() || !k.is<std::string>()) {
                continue;
              }

              sol::table t = v.as<sol::table>();
              std::string name = k.as<std::string>();

              if (name == "logins") {
                for (auto &[_, x] : t) {
                  if (x.is<std::string>()) {
                    logins.push_back(x.as<std::string>());
                  }
                }
              } else if (name == "ids") {
                for (auto &[_, x] : t) {
                  if (x.is<long long>()) {
                    ids.push_back(x.as<long long>());
                  }
                }
              } else {
                throw std::runtime_error("Unknown key: " + name);
              }
            }

            if (ids.empty() && logins.empty()) {
              throw std::runtime_error("No IDs or logins to search for.");
            }

            auto users = bundle.helix_client.get_users(ids, logins);

            sol::table o = state->create_table();

            std::for_each(users.begin(), users.end(),
                          [state, &o](const api::twitch::schemas::User &x) {
                            sol::table u = state->create_table();

                            u["id"] = x.id;
                            u["login"] = x.login;

                            o.add(u);
                          });

            return o;
          });
    }

    void add_kick_library(std::shared_ptr<sol::state> state,
                          const InstanceBundle &bundle) {
      state->set_function(
          "kick_get_channels", [state, &bundle](const std::string &slug) {
            std::vector<api::KickChannel> channels =
                bundle.kick_api_client.get_channels(slug);

            sol::table o = state->create_table();

            std::for_each(channels.begin(), channels.end(),
                          [state, &o](const api::KickChannel &x) {
                            sol::table u = state->create_table();

                            u["id"] = x.broadcaster_user_id;
                            u["login"] = x.slug;

                            o.add(u);
                          });

            return o;
          });
    }

    void add_storage_library(std::shared_ptr<sol::state> state,
                             const Request &request, const Configuration &cfg,
                             const std::string &lua_id) {
      state->set_function("storage_get", [state, &request, &cfg, &lua_id]() {
        std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);
        std::vector<std::string> params{
            std::to_string(request.requester.user.get_id()), lua_id};

        db::DatabaseRows rows = conn->exec(
            "SELECT value FROM lua_user_storage WHERE user_id = $1 AND "
            "lua_id = $2",
            params);

        std::string value = "";

        if (rows.empty()) {
          conn->exec(
              "INSERT INTO lua_user_storage(user_id, lua_id) VALUES ($1, "
              "$2)",
              params);
        } else {
          value = rows[0].at("value");
        }

        return value;
      });

      state->set_function("storage_put", [state, &request, &cfg,
                                          &lua_id](const std::string &value) {
        std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);
        std::vector<std::string> params{
            std::to_string(request.requester.user.get_id()), lua_id};

        db::DatabaseRows rows = conn->exec(
            "SELECT id FROM lua_user_storage WHERE user_id = $1 AND "
            "lua_id = $2",
            params);

        if (rows.empty()) {
          params.push_back(value);
          conn->exec(
              "INSERT INTO lua_user_storage(user_id, lua_id, value) VALUES "
              "($1, "
              "$2, $3)",
              params);
        } else {
          conn->exec("UPDATE lua_user_storage SET value = $1 WHERE id = $2",
                     {value, rows[0].at("id")});
        }

        return true;
      });

      state->set_function("storage_channel_get", [state, &request, &cfg,
                                                  &lua_id]() {
        std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);
        std::vector<std::string> params{
            std::to_string(request.requester.channel.get_id()), lua_id};

        db::DatabaseRows rows = conn->exec(
            "SELECT value FROM lua_channel_storage WHERE channel_id = $1 AND "
            "lua_id = $2",
            params);

        std::string value = "";

        if (rows.empty()) {
          conn->exec(
              "INSERT INTO lua_channel_storage(channel_id, lua_id) VALUES ($1, "
              "$2)",
              params);
        } else {
          value = rows[0].at("value");
        }

        return value;
      });

      state->set_function(
          "storage_channel_put",
          [state, &request, &cfg, &lua_id](const std::string &value) {
            std::unique_ptr<db::BaseDatabase> conn = db::create_connection(cfg);
            std::vector<std::string> params{
                std::to_string(request.requester.channel.get_id()), lua_id};

            db::DatabaseRows rows = conn->exec(
                "SELECT id FROM lua_channel_storage WHERE channel_id = $1 AND "
                "lua_id = $2",
                params);

            if (rows.empty()) {
              params.push_back(value);
              conn->exec(
                  "INSERT INTO lua_channel_storage(channel_id, lua_id, value) "
                  "VALUES "
                  "($1, "
                  "$2, $3)",
                  params);
            } else {
              conn->exec(
                  "UPDATE lua_channel_storage SET value = $1 WHERE id = $2",
                  {value, rows[0].at("id")});
            }

            return true;
          });
    }
  }

  Response parse_lua_response(const sol::table &r, sol::object &res,
                              bool moon_prefix = true) {
    const std::string prefix = moon_prefix ? "🌑 " : "";

    if (res.get_type() == sol::type::function) {
      sol::function f = res.as<sol::function>();
      sol::object o = f(r);
      return parse_lua_response(r, o);
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
                                        std::string lua_id) {
    // shared_ptr is unnecessary here, but my library needs it.
    std::shared_ptr<sol::state> state = std::make_shared<sol::state>();

    state->open_libraries(sol::lib::base, sol::lib::table, sol::lib::string,
                          sol::lib::math);
    library::add_base_libraries(state);

    if (!lua_id.empty()) {
      library::add_storage_library(state, request, bundle.configuration,
                                   lua_id);
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