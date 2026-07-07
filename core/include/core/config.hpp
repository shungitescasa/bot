#pragma once

#include <string>
namespace bot {
  struct IRCConfiguration {
      std::string host, port, nick, pass;
  };

  struct RPCConfiguration {
      std::string host = "127.0.0.1";
      unsigned int port = 3002;
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
  };
}