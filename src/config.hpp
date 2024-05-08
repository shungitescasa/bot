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

  struct TwitchCredentialsConfiguration {
      std::string client_id;
      std::string token;
  };

  struct CommandConfiguration {
      bool join_allowed = true;
      bool join_allow_from_other_chats = false;
  };

  struct Configuration {
      TwitchCredentialsConfiguration twitch_credentials;
      DatabaseConfiguration database;
      CommandConfiguration commands;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
