#pragma once

#include <string>
namespace bot {
  struct IRCConfiguration {
      std::string host, port, nick, pass;
  };

  struct Configuration {
      IRCConfiguration irc;

      Configuration() = default;
      Configuration(const Configuration &) = delete;
      Configuration &operator=(const Configuration &) = delete;

      static Configuration &get_instance() {
        static Configuration instance;
        return instance;
      }

      void load_file(const std::string &file_path);
  };
}