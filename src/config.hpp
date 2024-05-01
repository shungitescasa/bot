#pragma once

#include <optional>
#include <string>

#define GET_DATABASE_CONNECTION_URL(c)                                      \
  "dbname = " + c.database.name + " user = " + c.database.user +            \
      " password = " + c.database.password + " host = " + c.database.host + \
      " port = " + c.database.port

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
      std::string bot_client_id;
      DatabaseConfiguration database;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
