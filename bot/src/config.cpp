#include "config.hpp"

#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "logger.hpp"
#include "utils/string.hpp"

namespace bot {
  sol::table Configuration::as_lua_table(
      std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    // we parse only safe-to-leak parts of configuration

    // --- COMMAND
    sol::table cmds = luaState->create_table();
    cmds["join_allowed"] = this->commands.join_allowed;
    cmds["join_allow_from_other_chats"] =
        this->commands.join_allow_from_other_chats;
    if (this->commands.rpost_path.has_value()) {
      cmds["rpost_path"] = this->commands.rpost_path.value();
    } else {
      cmds["rpost_path"] = sol::lua_nil;
    }
    if (this->commands.paste_path.has_value()) {
      cmds["paste_path"] = this->commands.paste_path.value();
    } else {
      cmds["paste_path"] = sol::lua_nil;
    }
    cmds["paste_body_name"] = this->commands.paste_body_name;
    cmds["paste_title_name"] = this->commands.paste_title_name;
    o["commands"] = cmds;

    // --- OWNER
    sol::table owner = luaState->create_table();
    if (this->owner.name.has_value()) {
      owner["name"] = this->owner.name.value();
    } else {
      owner["name"] = sol::lua_nil;
    }
    if (this->owner.id.has_value()) {
      owner["id"] = this->owner.id.value();
    } else {
      owner["id"] = sol::lua_nil;
    }
    o["owner"] = owner;

    // --- URL
    sol::table url = luaState->create_table();
    if (this->url.help.has_value()) {
      url["help"] = this->url.help.value();
    } else {
      url["help"] = sol::lua_nil;
    }
    if (this->url.paste_service.has_value()) {
      url["paste_service"] = this->url.paste_service.value();
    } else {
      url["paste_service"] = sol::lua_nil;
    }
    if (this->url.randompost.has_value()) {
      url["randompost"] = this->url.randompost.value();
    } else {
      url["randompost"] = sol::lua_nil;
    }
    o["url"] = url;

    sol::table rss = luaState->create_table();
    if (this->rss.bridge.has_value()) {
      rss["bridge"] = this->rss.bridge.value();
    } else {
      rss["bridge"] = sol::lua_nil;
    }
    rss["timeout"] = this->rss.timeout;

    o["rss"] = rss;

    return o;
  }

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path) {
    std::ifstream ifs(file_path);

    if (!ifs.is_open()) {
      log::error("Configuration", "Failed to open the file at " + file_path);
      return std::nullopt;
    }

    Configuration cfg;
    TwitchConfiguration ttv_cfg;
    KickCredentialsConfiguration kick_crd_cfg;
    DatabaseConfiguration db_cfg;
    CommandConfiguration cmd_cfg;
    OwnerConfiguration owner_cfg;
    UrlConfiguration url_cfg;
    TokenConfiguration token_cfg;
    RssConfiguration rss_cfg;
    LuaConfiguration lua_cfg;

    std::string line;
    while (std::getline(ifs, line, '\n')) {
      std::istringstream iss(line);
      std::string key;
      std::string value;

      std::getline(iss, key, '=');
      std::getline(iss, value);

      for (char &c : key) {
        c = tolower(c);
      }

      if (key == "twitch.client_id") {
        ttv_cfg.client_id = value;
      } else if (key == "twitch.token") {
        ttv_cfg.token = value;
      } else if (key == "twitch.trusted_user_ids") {
        for (const std::string &x : utils::string::split_text(value, ',')) {
          ttv_cfg.trusted_user_ids.push_back(std::stoi(x));
        }
      }

      else if (key == "db_name") {
        db_cfg.name = value;
      } else if (key == "db_user") {
        db_cfg.user = value;
      } else if (key == "db_password") {
        db_cfg.password = value;
      } else if (key == "db_host") {
        db_cfg.host = value;
      } else if (key == "db_port") {
        db_cfg.port = value;
      }

      else if (key == "kick.client_id") {
        kick_crd_cfg.client_id = value;
      } else if (key == "kick.client_secret") {
        kick_crd_cfg.client_secret = value;
      }

      else if (key == "commands.join_allowed") {
        cmd_cfg.join_allowed = std::stoi(value);
      } else if (key == "commands.join_allow_from_other_chats") {
        cmd_cfg.join_allow_from_other_chats = std::stoi(value);
      } else if (key == "commands.randompost.path") {
        cmd_cfg.rpost_path = value;
      } else if (key == "commands.paste_path") {
        cmd_cfg.paste_path = value;
      } else if (key == "paste_body_name") {
        cmd_cfg.paste_body_name = value;
      } else if (key == "paste_title_name") {
        cmd_cfg.paste_title_name = value;
      }

      else if (key == "owner.name") {
        owner_cfg.name = value;
      } else if (key == "owner.id") {
        owner_cfg.id = std::stoi(value);
      }

      else if (key == "url.help") {
        url_cfg.help = value;
      } else if (key == "url.chatters.paste_service") {
        url_cfg.paste_service = value;
      } else if (key == "url.randompost") {
        url_cfg.randompost = value;
      }

      else if (key == "rss.timeout") {
        rss_cfg.timeout = std::stoi(value);
      } else if (key == "rss.bridge") {
        rss_cfg.bridge = value;
      }

      else if (key == "lua.allow_arbitrary_scripts") {
        lua_cfg.allow_arbitrary_scripts = std::stoi(value);
      } else if (key == "lua.script_whitelist") {
        lua_cfg.script_whitelist = utils::string::split_text(value, ',');
      }

      else if (key == "token.github") {
        token_cfg.github_token = value;
      }
    }

    cfg.url = url_cfg;
    cfg.owner = owner_cfg;
    cfg.commands = cmd_cfg;
    cfg.twitch = ttv_cfg;
    cfg.kick_credentials = kick_crd_cfg;
    cfg.database = db_cfg;
    cfg.tokens = token_cfg;
    cfg.rss = rss_cfg;
    cfg.lua = lua_cfg;

    log::info("Configuration",
              "Successfully loaded the file from '" + file_path + "'");
    return cfg;
  }
}
