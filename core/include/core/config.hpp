#pragma once

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <format>
#include <memory>
#include <optional>
#include <sol/sol.hpp>
#include <string>

namespace bot {
  struct InstanceConfiguration {
      std::optional<std::string> name = std::nullopt;
      std::string user_agent = std::format(
          "tinybot/{} (compatible; "
          "https://wiki.shungites.casa/doku.php?id=bot:tinybot)",
          BOT_VERSION);
  };

  struct IRCConfiguration {
      std::string host, port, nick, pass;
  };

  struct RPCConfiguration {
      std::string host = "127.0.0.1", client_host = "127.0.0.1";
      unsigned int port = 3002, client_port = 3003;
  };

  struct ScriptConfiguration {
      std::string loader = "lua", directory = "luascripts";
      unsigned int timeout = 0;
  };

  struct DatabaseConfiguration {
      std::string host = "127.0.0.1", name = "bot";
      std::string user = "default", password = "default";
#if USE_POSTGRES
      unsigned int port = 5432;
#else
      unsigned int port = 3306;
#endif
  };

  struct Configuration {
      InstanceConfiguration instance;
      IRCConfiguration irc;
      RPCConfiguration rpc;
      ScriptConfiguration script;
      DatabaseConfiguration database;

      Configuration() = default;
      Configuration(const Configuration &) = delete;
      Configuration &operator=(const Configuration &) = delete;

      static Configuration &get_instance() {
        static Configuration instance;
        return instance;
      }

      void load_file(const std::string &file_path);
      void load_from_args(int argc, char *argv[]);

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const;
  };
}
