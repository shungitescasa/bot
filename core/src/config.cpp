#include "core/config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bot {
  void Configuration::load_file(const std::string &file_path) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
      throw std::runtime_error("Failed to open configuration file (" +
                               file_path + ")");
    }

    std::string line;
    while (std::getline(ifs, line, '\n')) {
      if (line.empty()) continue;
      std::istringstream iss(line);
      std::string key, value;

      std::getline(iss, key, '=');
      std::getline(iss, value);

      if (key == "irc.host")
        irc.host = value;
      else if (key == "irc.port")
        irc.port = value;
      else if (key == "irc.nick")
        irc.nick = value;
      else if (key == "irc.pass")
        irc.pass = value;
    }
  }
}