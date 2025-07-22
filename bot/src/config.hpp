#pragma once

#include <optional>
#include <sol/sol.hpp>
#include <string>

#define GET_DATABASE_CONNECTION_URL(c)                                      \
  "dbname = " + c.database.name + " user = " + c.database.user +            \
      " password = " + c.database.password + " host = " + c.database.host + \
      " port = " + c.database.port

#define GET_DATABASE_CONNECTION_URL_POINTER(c)                                \
  "dbname = " + c->database.name + " user = " + c->database.user +            \
      " password = " + c->database.password + " host = " + c->database.host + \
      " port = " + c->database.port

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

  struct KickCredentialsConfiguration {
      std::string client_id, client_secret;
  };

  struct CommandConfiguration {
      bool join_allowed = true;
      bool join_allow_from_other_chats = false;

      std::optional<std::string> rpost_path = std::nullopt;

      std::optional<std::string> paste_path = std::nullopt;
      std::string paste_body_name = "paste";
      std::string paste_title_name = "title";
  };

  struct OwnerConfiguration {
      std::optional<std::string> name = std::nullopt;
      std::optional<int> id = std::nullopt;
  };

  struct UrlConfiguration {
      std::optional<std::string> help = std::nullopt;
      std::optional<std::string> paste_service = std::nullopt;
      std::optional<std::string> randompost = std::nullopt;
      std::optional<std::string> rssbridge = std::nullopt;
  };

  struct TokenConfiguration {
      std::optional<std::string> github_token = std::nullopt;
  };

  struct Configuration {
      TwitchCredentialsConfiguration twitch_credentials;
      KickCredentialsConfiguration kick_credentials;
      DatabaseConfiguration database;
      CommandConfiguration commands;
      OwnerConfiguration owner;
      UrlConfiguration url;
      TokenConfiguration tokens;

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
