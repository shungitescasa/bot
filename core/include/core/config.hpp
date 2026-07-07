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

  struct Configuration {
      IRCConfiguration irc;
      RPCConfiguration rpc;

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