#pragma once

#include <optional>
#include <string>

namespace bot {
  struct DatabaseConfiguration {
      std::string name;
      std::string user;
      std::string password;
      std::string host;
      std::string port;
  };

  struct Configuration {
      std::string bot_username;
      std::string bot_password;
      DatabaseConfiguration database;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
