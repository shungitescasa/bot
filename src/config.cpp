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

      if (key == "bot_username") {
        cfg.bot_username = value;
      } else if (key == "bot_password") {
        cfg.bot_password = value;
      }
    }

    return cfg;
  }
}
