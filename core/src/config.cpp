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

      else if (key == "rpc.host")
        rpc.host = value;
      else if (key == "rpc.port")
        rpc.port = std::stoi(value);
    }
  }

  void Configuration::load_from_args(int argc, char *argv[]) {
    std::string config_path = ".env";

    for (int i = 0; i < argc; i++) {
      if (i + 1 <= argc - 1) {
        std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--config" || k == "-c") config_path = v;
      }
    }

    load_file(config_path);
  }
}