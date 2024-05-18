#include "config.hpp"

#include <fstream>
#include <sstream>

#include "crow/logging.h"

namespace botweb {
  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path) {
    std::ifstream ifs(file_path);

    if (!ifs.is_open()) {
      CROW_LOG_ERROR << "Failed to open the configuration file at "
                     << file_path;
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

      if (key == "contact_name") {
        cfg.contact_name = value;
      } else if (key == "contact_url") {
        cfg.contact_url = value;
      }
    }

    CROW_LOG_INFO << "Successfully loaded the configuration from " << file_path
                  << "'";

    return cfg;
  }
}
