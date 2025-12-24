#pragma once

#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <vector>

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

  struct TwitchConfiguration {
      unsigned int user_id;
      std::string client_id;
      std::string token;
      std::vector<int> trusted_user_ids, superuser_ids;
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

  struct UrlConfiguration {
      std::optional<std::string> help = std::nullopt;
      std::optional<std::string> paste_service = std::nullopt;
      std::optional<std::string> randompost = std::nullopt;
      std::optional<std::string> stats = std::nullopt;
  };

  struct TokenConfiguration {
      std::optional<std::string> github_token = std::nullopt;
  };

  struct RssConfiguration {
      std::optional<std::string> bridge = std::nullopt;
      int timeout = 60;
  };

  struct LuaConfiguration {
      bool allow_arbitrary_scripts = false;
      std::vector<std::string> script_whitelist;
  };

  struct Configuration {
      TwitchConfiguration twitch;
      KickCredentialsConfiguration kick_credentials;
      DatabaseConfiguration database;
      CommandConfiguration commands;
      UrlConfiguration url;
      TokenConfiguration tokens;
      RssConfiguration rss;
      LuaConfiguration lua;

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
