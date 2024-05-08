#include "config.hpp"

#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace bot {
  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path) {
    std::ifstream ifs(file_path);

    if (!ifs.is_open()) {
      std::cerr << "*** Failed to open the configuration file: " << file_path
                << "!\n";
      return std::nullopt;
    }

    Configuration cfg;
    TwitchCredentialsConfiguration ttv_crd_cfg;
    DatabaseConfiguration db_cfg;
    CommandConfiguration cmd_cfg;
    OwnerConfiguration owner_cfg;

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

      if (key == "twitch_credentials.client_id") {
        ttv_crd_cfg.client_id = value;
      } else if (key == "twitch_credentials.token") {
        ttv_crd_cfg.token = value;
      } else if (key == "db_name") {
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

      else if (key == "commands.join_allowed") {
        cmd_cfg.join_allowed = std::stoi(value);
      } else if (key == "commands.join_allow_from_other_chats") {
        cmd_cfg.join_allow_from_other_chats = std::stoi(value);
      }

      else if (key == "owner.name") {
        owner_cfg.name = value;
      } else if (key == "owner.id") {
        owner_cfg.id = std::stoi(value);
      }
    }

    cfg.owner = owner_cfg;
    cfg.commands = cmd_cfg;
    cfg.twitch_credentials = ttv_crd_cfg;
    cfg.database = db_cfg;

    return cfg;
  }
}
