#include "request.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "../constants.hpp"

namespace bot {
  namespace command {
    bool Request::fill_request() {
      std::vector<std::string> parts =
          utils::string::split_text(irc_message.message, ' ');

      std::string command_id = parts[0];

      if (command_id.substr(0, DEFAULT_PREFIX.length()) != DEFAULT_PREFIX) {
        return false;
      }

      command_id =
          command_id.substr(DEFAULT_PREFIX.length(), command_id.length());

      bool found = std::any_of(this->command_loader.get_commands().begin(),
                               this->command_loader.get_commands().end(),
                               [&](const auto &command) {
                                 return command->get_name() == command_id;
                               });

      if (!found) {
        return false;
      }

      this->command_id = command_id;

      parts.erase(parts.begin());

      if (parts.empty()) {
        return true;
      }

      std::string subcommand_id = parts[0];
      if (subcommand_id.empty()) {
        this->subcommand_id = std::nullopt;
      } else {
        this->subcommand_id = subcommand_id;
      }
      parts.erase(parts.begin());

      std::string message = utils::string::join_vector(parts, ' ');

      if (message.empty()) {
        this->message = std::nullopt;
      } else {
        this->message = message;
      }

      return true;
    }
  }
}
