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

      if (key == "instance.name")
        instance.name = value;
      else if (key == "instance.user_agent")
        instance.user_agent = value;

      else if (key == "irc.host")
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
      else if (key == "rpc.client_host")
        rpc.client_host = value;
      else if (key == "rpc.client_port")
        rpc.client_port = std::stoi(value);

      else if (key == "script.loader")
        script.loader = value;
      else if (key == "script.directory")
        script.directory = value;
      else if (key == "script.timeout")
        script.timeout = std::stoi(value);

      else if (key == "database.host")
        database.host = value;
      else if (key == "database.name")
        database.name = value;
      else if (key == "database.user")
        database.user = value;
      else if (key == "database.password")
        database.password = value;
      else if (key == "database.port")
        database.port = std::stoi(value);
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

  sol::table Configuration::as_lua_table(
      std::shared_ptr<sol::state> state) const {
    sol::table o = state->create_table();

    return o;
  }
}
