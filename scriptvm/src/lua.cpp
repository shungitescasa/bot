#include "scriptvm/lua.hpp"

#include <cpr/cpr.h>
#include <sys/resource.h>

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sol/object.hpp>
#include <sol/table.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/bot.hpp"
#include "core/builtin.hpp"
#include "core/command.hpp"
#include "core/config.hpp"
#include "core/message.hpp"
#include "core/rss.hpp"
#include "core/utils.hpp"
#include "emotespp/seventv.hpp"

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
    libraries::open_extended_libraries(this->lua, this);
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
    sol::table r = request.as_lua_table(this->state);
    sol::object res = this->handle(r);
    return parse_lua_response(r, res, false);
  }

  bot::Response run_safe_lua_script(const bot::Request &request,
                                    const std::string &script,
                                    std::string lua_id, bool moon_prefix) {
    // shared_ptr is unnecessary here, but my library needs it.
    std::shared_ptr<sol::state> state = std::make_shared<sol::state>();

    state->open_libraries(sol::lib::base, sol::lib::table, sol::lib::string,
                          sol::lib::math);
    libraries::open_base_libraries(state, nullptr);

    if (!lua_id.empty()) {
      libraries::open_storage_library(state, request.requester, lua_id);
    }

    sol::load_result s = state->load("return " + script);
    if (!s.valid()) {
      s = state->load(script);
    }

    if (!s.valid()) {
      sol::error err = s;
      throw std::runtime_error("Bad Lua script");
    }

    sol::protected_function_result res = s();

    if (!res.valid()) {
      sol::error err = s;
      throw std::runtime_error("Lua script execution fail");
    }

    sol::object o = res;

    return parse_lua_response(request.as_lua_table(state), o, moon_prefix);
  }

  namespace libraries {
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

    void open_bot_library(std::shared_ptr<sol::state> state,
                          LuaScriptLoader *loader) {
      state->set_function("bot_get_compiler_version", []() {
        std::string info;

#ifdef __cplusplus
        info.append(
            std::format("C++{}", std::to_string(__cplusplus).substr(2, 2)));
#endif

#ifdef __VERSION__
        info.append(std::format(" ({})", __VERSION__));
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
        bot::Configuration &cfg = bot::Configuration::get_instance();
        return cfg.as_lua_table(state);
      });

      state->set_function("bot_username", []() {
        return bot::Configuration::get_instance().instance.name;
      });

      state->set_function("bot_get_loaded_command_names", [state, loader]() {
        sol::table o = state->create_table();
        if (loader == nullptr) return o;

        for (bot::CommandBox cmd : loader->get_commands())
          o.add(cmd->data().name);
        return o;
      });
    }

    void open_time_library(std::shared_ptr<sol::state> state) {
      state->set_function("time_current", []() {
        return static_cast<long long>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
      });

      state->set_function("time_humanize", [](const int &timestamp) {
        return bot::utils::chrono::humanize_timestamp(timestamp);
      });

      state->set_function("time_humanize", [](const double &timestamp) {
        return bot::utils::chrono::humanize_timestamp(std::floor(timestamp));
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

    void open_string_library(std::shared_ptr<sol::state> state) {
      state->set_function("str_format",
                          [](const std::string &str, const sol::table &params) {
                            // TODO: C++23 code
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
            for (const auto &part : std::ranges::views::split(text, delimiter))
              o.add(std::string(part.begin(), part.end()));
            return o;
          });

      // TODO
      state->set_function("event_type_to_str", [](const int &v) { return ""; });

      state->set_function("str_to_event_type",
                          [](const std::string &v) { return 0; });

      state->set_function("str_to_event_flag",
                          [state](const std::string &v) { return 0; });

      state->set_function("event_flag_to_str",
                          [state](const int &v) { return ""; });

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

      state->set_function("str_startswith", [state](const std::string &haystack,
                                                    const std::string &needle) {
        return haystack.starts_with(needle);
      });
    }

    void open_l10n_library(std::shared_ptr<sol::state> state) {
      state->set_function(
          "l10n_custom_formatted_line_request",
          [](const sol::table &request, const sol::table &lines,
             const std::string &line_id, const sol::table &parameters) {
            // TODO: use Localization class instead!!!

            // TODO: convert the table to C++ struct for type safety later
            std::string language = request["room_preference"]["language"];

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

      state->set_function("l10n_get_localization_names", [state]() {
        sol::table o = state->create_table();
        o.add("russian");
        o.add("english");
        return o;
      });
    }

    void open_array_library(std::shared_ptr<sol::state> state) {
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

    void open_json_library(std::shared_ptr<sol::state> state) {
      state->set_function("json_parse", [state](const std::string &s) {
        nlohmann::json j = nlohmann::json::parse(s);
        return parse_json_object(state, j);
      });

      state->set_function("json_stringify", [](const sol::object &o) {
        return lua_to_json(o).dump();
      });

      state->set_function("json_get_value", [state](const sol::object &body,
                                                    const std::string &path) {
        nlohmann::json o = lua_to_json(body);
        for (const auto &part : std::ranges::views::split(path, '.'))
          o = o[std::string(part.begin(), part.end())];
        return parse_json_object(state, o);
      });
    }

    void open_database_library(std::shared_ptr<sol::state> state) {
      state->set_function("db_execute", [state](const std::string &query,
                                                const sol::table &parameters) {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();

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

      state->set_function("db_query", [state](const std::string &query,
                                              const sol::table &parameters) {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();

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

        bot::data::DatabaseRows rows = conn->exec(query, params);

        sol::table o = state->create_table();

        for (const bot::data::DatabaseRow &row : rows) {
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

    void open_network_library(std::shared_ptr<sol::state> state) {
      state->set_function("net_get", [state](const std::string &url) {
        sol::table t = state->create_table();

        bot::Configuration &cfg = bot::Configuration::get_instance();
        cpr::Response response =
            cpr::Get(cpr::Url{url},
                     cpr::Header{{"User-Agent", cfg.instance.user_agent}});

        t["code"] = response.status_code;
        t["text"] = response.text;

        return t;
      });

      state->set_function(
          "net_get_with_headers",
          [state](const std::string &url, const sol::table &headers) {
            bot::Configuration &cfg = bot::Configuration::get_instance();
            sol::table t = state->create_table();

            cpr::Header h{};

            for (auto &kv : headers) {
              h[kv.first.as<std::string>()] = kv.second.as<std::string>();
            }
            h["User-Agent"] = cfg.instance.user_agent;

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

            bot::Configuration &cfg = bot::Configuration::get_instance();

            cpr::Response response =
                cpr::Post(cpr::Url{url}, multipart,
                          cpr::Header{{"User-Agent", cfg.instance.user_agent}});

            t["code"] = response.status_code;
            t["text"] = response.text;

            return t;
          });

      state->set_function(
          "net_post_multipart_with_headers",
          [state](const std::string &url, const sol::table &body,
                  const sol::table &headers) {
            bot::Configuration &cfg = bot::Configuration::get_instance();
            sol::table t = state->create_table();

            cpr::Header h{};

            for (auto &kv : headers) {
              h[kv.first.as<std::string>()] = kv.second.as<std::string>();
            }
            h["User-Agent"] = cfg.instance.user_agent;

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

      state->set_function("paste_upload", [state](const std::string &contents,
                                                  const sol::object &subject) {
        bot::Configuration &cfg = bot::Configuration::get_instance();

        cpr::Multipart multipart = {{cfg.anonbin.contents, contents}};
        if (subject.is<std::string>()) {
          multipart.parts.push_back(
              {cfg.anonbin.subject, subject.as<std::string>()});
        }

        cpr::Response response =
            cpr::Post(cpr::Url{*cfg.anonbin.url}, multipart,
                      cpr::Header{{"Accept", "application/json"},
                                  {"User-Agent", cfg.instance.user_agent}});

        if (response.status_code >= 400) {
          throw std::runtime_error("Failed to upload paste: " +
                                   std::to_string(response.status_code));
        }

        nlohmann::json o = nlohmann::json::parse(response.text);
        for (const auto &part :
             std::ranges::views::split(cfg.anonbin.path, '.'))
          o = o[std::string(part.begin(), part.end())];
        return parse_json_object(state, o);
      });

      state->set_function(
          "image_upload_base64", [state](const std::string &base64) {
            bot::Configuration &cfg = bot::Configuration::get_instance();

            cpr::Response response = cpr::Post(
                cpr::Url{*cfg.anonupload.url},
                cpr::Multipart{{cfg.anonupload.base64_contents, base64}},
                cpr::Header{{"Accept", "application/json"},
                            {"User-Agent", cfg.instance.user_agent}});

            if (response.status_code >= 400) {
              throw std::runtime_error("Failed to upload image: " +
                                       std::to_string(response.status_code));
            }

            nlohmann::json o = nlohmann::json::parse(response.text);
            for (const auto &part :
                 std::ranges::views::split(cfg.anonbin.path, '.'))
              o = o[std::string(part.begin(), part.end())];
            return parse_json_object(state, o);
          });

      state->set_function("image_random", [state]() {
        bot::Configuration &cfg = bot::Configuration::get_instance();

        cpr::Response response =
            cpr::Get(cpr::Url{*cfg.anonupload.url + "/?random"},
                     cpr::Header{{"Accept", "application/json"},
                                 {"User-Agent", cfg.instance.user_agent}});

        if (response.status_code >= 400) {
          throw std::runtime_error("Failed to get a random image: " +
                                   std::to_string(response.status_code));
        }

        nlohmann::json o = nlohmann::json::parse(response.text);
        for (const auto &part :
             std::ranges::views::split(cfg.anonbin.path, '.'))
          o = o[std::string(part.begin(), part.end())];
        return parse_json_object(state, o);
      });
    }

    void open_event_library(std::shared_ptr<sol::state> state) {
      state->set_function("events_get", [state](const std::string &type,
                                                const std::string &name) {
        bot::RSSEvent event(type, name);
        std::vector<bot::RSSItem> items = event.fetch_items();

        sol::table o = state->create_table();
        for (bot::RSSItem item : items) o.add(item.as_lua_table(state));
        return o;
      });

      state->set_function("is_valid_event", [state](const std::string &type,
                                                    const std::string &name) {
        try {
          bot::RSSEvent event(type, name);
          return true;
        } catch (std::exception &e) {
          return false;
        }
      });

      state->set_function("events_parse_target",
                          [state](const std::string &input) {
                            int pos = input.rfind(':');
                            if (pos == std::string::npos) {
                              return sol::make_object(*state, sol::lua_nil);
                            }

                            sol::table o = state->create_table();
                            o["name"] = input.substr(0, pos);
                            o["type"] = input.substr(pos + 1);
                            return sol::make_object(*state, o);
                          });
    }

    void open_emote_library(std::shared_ptr<sol::state> state) {
      auto &cfg = bot::Configuration::get_instance();
      if (!cfg.seventv.key.has_value()) {
        return;
      }

      emotespp::SevenTVAPIClient client{cfg.seventv.key.value()};

      auto user_to_lua = [state](const emotespp::User &user) {
        auto o = state->create_table();
        o["username"] = user.username;
        o["alias_id"] = user.alias_id;
        o["emote_set_id"] = user.emote_set_id;
        o["id"] = user.id;
        return o;
      };

      auto emotes_to_lua = [state](const std::vector<emotespp::Emote> &emotes) {
        auto e = state->create_table();
        for (int i = 0; i < emotes.size(); i++) {
          auto emote = emotes[i];
          auto em = state->create_table();
          em["id"] = emote.id;
          em["code"] = emote.code;
          if (emote.original_code.has_value()) {
            em["original_code"] = *emote.original_code;
          } else {
            em["original_code"] = sol::lua_nil;
          }
          e[i + 1] = em;
        }
        return e;
      };

      state->set_function(
          "stv_add_named_emote",
          [client](const std::string &emote_set_id, const std::string &emote_id,
                   const std::string &name) {
            client.add_emote(emote_set_id, {emote_id, name});
          });

      state->set_function("stv_add_emote",
                          [client](const std::string &emote_set_id,
                                   const std::string &emote_id) {
                            client.add_emote(emote_set_id, {emote_id, ""});
                          });

      state->set_function("stv_remove_emote",
                          [client](const std::string &emote_set_id,
                                   const std::string &emote_id) {
                            client.remove_emote(emote_set_id, {emote_id});
                          });

      state->set_function(
          "stv_rename_emote",
          [client](const std::string &emote_set_id, const std::string &emote_id,
                   const std::string &new_name) {
            client.rename_emote(emote_set_id, {emote_id, new_name});
          });

      state->set_function("stv_get_user", [state, client, user_to_lua](
                                              const unsigned int &twitch_id) {
        auto user = client.get_user_by_twitch_id(twitch_id);

        if (!user.has_value()) {
          return sol::make_object(*state, sol::lua_nil);
        }

        return sol::make_object(*state, user_to_lua(*user));
      });

      state->set_function("stv_get_emoteset",
                          [state, client, user_to_lua,
                           emotes_to_lua](const std::string &emote_set_id) {
                            auto set = client.get_emote_set(emote_set_id);

                            if (!set.has_value()) {
                              return sol::make_object(*state, sol::lua_nil);
                            }

                            auto o = state->create_table();
                            o["id"] = set->id;
                            o["name"] = set->name;
                            o["owner"] = user_to_lua(set->owner);
                            o["emotes"] = emotes_to_lua(set->emotes);

                            return sol::make_object(*state, o);
                          });

      state->set_function("stv_search_emotes", [state, client, emotes_to_lua](
                                                   const std::string &name) {
        return emotes_to_lua(client.search_emotes(name));
      });
    }

    void open_irc_library(std::shared_ptr<sol::state> state) {
      state->set_function("irc_join_channel", [](const sol::table &room) {
        bot::RPCChatBot::get_instance().join({room});
      });

      state->set_function("irc_send_message", [](const sol::table &room,
                                                 const std::string &message) {
        bot::RPCChatBot::get_instance().send_message({room}, message);
      });

      state->set_function("irc_part_channel", [](const sol::table &room) {
        bot::RPCChatBot::get_instance().part({room});
      });
    }

    void open_twitch_library(std::shared_ptr<sol::state> state) {
      // TODO: ratelimits
      state->set_function(
          "twitch_get_chatters", [state](const int &broadcaster_id) {
            auto &chatbot = bot::RPCChatBot::get_instance();
            auto users =
                bot::RPCChatBot::get_instance().get_chatters(broadcaster_id);

            sol::table o = state->create_table();

            for (auto &user : users) {
              sol::table u = state->create_table();
              u["id"] = user.id;
              u["login"] = user.login;
              o.add(u);
            }

            return o;
          });

      state->set_function("twitch_get_users", [state](const sol::table &names) {
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

        auto users = bot::RPCChatBot::get_instance().get_users(ids, logins);

        sol::table o = state->create_table();

        for (auto &user : users) {
          sol::table u = state->create_table();
          u["id"] = user.id;
          u["login"] = user.login;
          o.add(u);
        }

        return o;
      });

      state->set_function("twitch_get_global_emotes", [state]() {
        auto emotes = bot::RPCChatBot::get_instance().get_global_emotes();
        sol::table o = state->create_table();

        for (auto emote : emotes) {
          sol::table e = state->create_table();
          e["id"] = emote.id;
          e["name"] = emote.code;
          o.add(e);
        }

        return o;
      });

      state->set_function("twitch_get_channel_emotes", [state](const int &id) {
        auto emotes = bot::RPCChatBot::get_instance().get_channel_emotes(id);
        sol::table o = state->create_table();

        for (auto emote : emotes) {
          sol::table e = state->create_table();
          e["id"] = emote.id;
          e["name"] = emote.code;
          o.add(e);
        }

        return o;
      });
    }

    void open_kick_library(std::shared_ptr<sol::state> state) {
      state->set_function("kick_get_channels",
                          [state](const std::vector<std::string> &slugs) {
                            return state->create_table();
                          });
    }

    void open_storage_library(std::shared_ptr<sol::state> state,
                              const bot::Requester &request,
                              const std::string &lua_id) {
      state->set_function("storage_get", [state, &request, &lua_id]() {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();
        std::vector<std::string> params{std::to_string(request.sender.id),
                                        lua_id};

        bot::data::DatabaseRows rows = conn->exec(
            "SELECT value FROM lua_sender_storage WHERE sender_id = $1 AND "
            "lua_id = $2",
            params);

        std::string value = "";

        if (rows.empty()) {
          conn->exec(
              "INSERT INTO lua_sender_storage(sender_id, lua_id) VALUES ($1, "
              "$2)",
              params);
        } else {
          value = rows[0].at("value");
        }

        return value;
      });

      state->set_function("storage_put", [state, &request,
                                          &lua_id](const std::string &value) {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();
        std::vector<std::string> params{std::to_string(request.sender.id),
                                        lua_id};

        bot::data::DatabaseRows rows = conn->exec(
            "SELECT id FROM lua_sender_storage WHERE sender_id = $1 AND "
            "lua_id = $2",
            params);

        if (rows.empty()) {
          params.push_back(value);
          conn->exec(
              "INSERT INTO lua_sender_storage(sender_id, lua_id, value) VALUES "
              "($1, "
              "$2, $3)",
              params);
        } else {
          conn->exec("UPDATE lua_sender_storage SET value = $1 WHERE id = $2",
                     {value, rows[0].at("id")});
        }

        return true;
      });

      state->set_function("storage_channel_get", [state, &request, &lua_id]() {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();
        std::vector<std::string> params{std::to_string(request.room.id),
                                        lua_id};

        bot::data::DatabaseRows rows = conn->exec(
            "SELECT value FROM lua_room_storage WHERE room_id = $1 AND "
            "lua_id = $2",
            params);

        std::string value = "";

        if (rows.empty()) {
          conn->exec(
              "INSERT INTO lua_room_storage(room_id, lua_id) VALUES ($1, "
              "$2)",
              params);
        } else {
          value = rows[0].at("value");
        }

        return value;
      });

      state->set_function("storage_channel_put", [state, &request, &lua_id](
                                                     const std::string &value) {
        std::unique_ptr<bot::data::BaseDatabase> conn =
            bot::data::create_connection();
        std::vector<std::string> params{std::to_string(request.room.id),
                                        lua_id};

        bot::data::DatabaseRows rows = conn->exec(
            "SELECT id FROM lua_room_storage WHERE room_id = $1 AND "
            "lua_id = $2",
            params);

        if (rows.empty()) {
          params.push_back(value);
          conn->exec(
              "INSERT INTO lua_room_storage(room_id, lua_id, value) "
              "VALUES "
              "($1, $2, $3)",
              params);
        } else {
          conn->exec("UPDATE lua_room_storage SET value = $1 WHERE id = $2",
                     {value, rows[0].at("id")});
        }

        return true;
      });
    }

    void open_base_libraries(std::shared_ptr<sol::state> state,
                             LuaScriptLoader *loader) {
      open_bot_library(state, loader);
      open_time_library(state);
      open_string_library(state);
      open_l10n_library(state);
      open_json_library(state);
      open_array_library(state);
      open_twitch_library(state);
      open_kick_library(state);
      open_emote_library(state);
    }

    void open_extended_libraries(std::shared_ptr<sol::state> state,
                                 LuaScriptLoader *loader) {
      open_base_libraries(state, loader);
      open_database_library(state);
      open_network_library(state);
      open_event_library(state);
      open_irc_library(state);
    }
  }
}
