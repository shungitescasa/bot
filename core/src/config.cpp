#include "core/config.hpp"

#include <fstream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bot {
  void Configuration::load_file(const std::string &file_path) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
      throw std::runtime_error("Failed to open configuration file (" +
                               file_path + ")");
    }

    std::string line;
    while (std::getline(ifs, line, '\n')) {
      if (line.empty()) continue;
      std::istringstream iss(line);
      std::string key, value;

      std::getline(iss, key, '=');
      std::getline(iss, value);

      if (key == "instance.name")
        instance.name = value;
      else if (key == "instance.host")
        instance.host = value;
      else if (key == "instance.user_agent")
        instance.user_agent = value;
      else if (key == "instance.supernicks") {
        for (const auto &x : std::ranges::views::split(value, ' ')) {
          instance.supernicks.push_back(std::string(x.begin(), x.end()));
        }
      }

      else if (key == "irc.host")
        irc.host = value;
      else if (key == "irc.port")
        irc.port = value;
      else if (key == "irc.nick")
        irc.nick = value;
      else if (key == "irc.pass")
        irc.pass = value;

      else if (key == "rpc.host")
        rpc.host = value;
      else if (key == "rpc.port")
        rpc.port = std::stoi(value);
      else if (key == "rpc.client_host")
        rpc.client_host = value;
      else if (key == "rpc.client_port")
        rpc.client_port = std::stoi(value);

      else if (key == "script.loader")
        script.loader = value;
      else if (key == "script.directory")
        script.directory = value;
      else if (key == "script.timeout")
        script.timeout = std::stoi(value);
      else if (key == "script.allow_arbitrary_scripts")
        script.allow_arbitrary_scripts = value == "true";
      else if (key == "script.url_whitelist") {
        for (const auto &x : std::ranges::views::split(value, ' ')) {
          script.url_whitelist.push_back(std::string(x.begin(), x.end()));
        }
      }

      else if (key == "database.host")
        database.host = value;
      else if (key == "database.name")
        database.name = value;
      else if (key == "database.user")
        database.user = value;
      else if (key == "database.password")
        database.password = value;
      else if (key == "database.port")
        database.port = std::stoi(value);

      else if (key == "twitch.token")
        twitch.token = value;

      else if (key == "join.allow_from_chat")
        join.allow_from_chat = value == "true";
      else if (key == "join.allow_other_origins")
        join.allow_other_origins = value == "true";

      else if (key == "anonbin.url")
        anonbin.url = value;
      else if (key == "anonbin.contents")
        anonbin.contents = value;
      else if (key == "anonbin.subject")
        anonbin.subject = value;
      else if (key == "anonbin.path")
        anonbin.path = value;

      else if (key == "anonupload.url")
        anonupload.url = value;
      else if (key == "anonupload.base64_contents")
        anonupload.base64_contents = value;

      else if (key == "7tv.key")
        seventv.key = value;

      else if (key == "tinyemotes.url")
        tinyemotes.url = value;

      else if (key == "rss.url")
        rss.url = value;
      else if (key == "rss.timeout")
        rss.timeout = std::stoi(value);

      else if (key == "thirdparty.mogranks")
        thirdparty.mogranks = value;
      else if (key == "thirdparty.stats")
        thirdparty.stats = value;
    }
  }

  void Configuration::load_from_args(int argc, char *argv[]) {
    std::string config_path = ".env";

    for (int i = 0; i < argc; i++) {
      if (i + 1 <= argc - 1) {
        std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--config" || k == "-c") config_path = v;
      }
    }

    load_file(config_path);
  }

  sol::table Configuration::as_lua_table(
      std::shared_ptr<sol::state> state) const {
    sol::table o = state->create_table();

    // instance
    {
      sol::table t = state->create_table();
      if (instance.name.has_value()) {
        t["name"] = instance.name.value();
      } else {
        t["name"] = sol::lua_nil;
      }
      if (instance.host.has_value()) {
        t["host"] = instance.host.value();
      } else {
        t["host"] = sol::lua_nil;
      }

      {
        sol::table n = state->create_table();
        for (const std::string &x : instance.supernicks) n.add(x);
        t["supernicks"] = n;
      }

      o["instance"] = std::move(t);
    }

    // tinyemotes
    {
      sol::table t = state->create_table();
      if (tinyemotes.url.has_value()) {
        t["url"] = tinyemotes.url.value();
      } else {
        t["url"] = sol::lua_nil;
      }
      o["tinyemotes"] = std::move(t);
    }

    // join
    {
      sol::table t = state->create_table();
      t["allow_from_chat"] = join.allow_from_chat;
      t["allow_other_origins"] = join.allow_other_origins;
      o["join"] = std::move(t);
    }

    // rss
    {
      sol::table t = state->create_table();
      if (rss.url.has_value()) {
        t["url"] = rss.url.value();
      } else {
        t["url"] = sol::lua_nil;
      }
      o["rss"] = std::move(t);
    }

    // thirdparty
    {
      sol::table t = state->create_table();
      if (thirdparty.mogranks.has_value()) {
        t["mogranks"] = thirdparty.mogranks.value();
      } else {
        t["mogranks"] = sol::lua_nil;
      }
      if (thirdparty.stats.has_value()) {
        t["stats"] = thirdparty.stats.value();
      } else {
        t["stats"] = sol::lua_nil;
      }
      o["thirdparty"] = std::move(t);
    }

    return o;
  }
}
