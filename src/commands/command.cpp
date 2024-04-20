#include "command.hpp"

#include <iostream>
#include <memory>
#include <optional>

#include "../modules/ping.hpp"

namespace bot {
  namespace command {
    CommandLoader::CommandLoader() {
      this->add_command(std::make_unique<mod::Ping>());
    }

    void CommandLoader::add_command(std::unique_ptr<Command> command) {
      this->commands.push_back(std::move(command));
    }

    std::optional<std::variant<std::vector<std::string>, std::string>>
    CommandLoader::run(const irc::Message<irc::MessageType::Privmsg> &msg) {
      for (const std::unique_ptr<Command> &command : this->commands) {
        if (command->get_name() == msg.message) {
          return command->run(msg);
        }
      }

      return std::nullopt;
    }
  }
}
