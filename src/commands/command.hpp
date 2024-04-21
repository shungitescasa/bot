#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../bundle.hpp"
#include "../irc/message.hpp"

namespace bot {
  namespace command {
    class Command {
      public:
        virtual std::string get_name() const = 0;
        virtual std::variant<std::vector<std::string>, std::string> run(
            const InstanceBundle &bundle,
            const irc::Message<irc::MessageType::Privmsg> &msg) const = 0;
    };

    class CommandLoader {
      public:
        CommandLoader();
        ~CommandLoader() = default;

        void add_command(std::unique_ptr<Command> cmd);
        std::optional<std::variant<std::vector<std::string>, std::string>> run(
            const InstanceBundle &bundle,
            const irc::Message<irc::MessageType::Privmsg> &msg) const;

        const std::vector<std::unique_ptr<Command>> &get_commands() const {
          return this->commands;
        };

      private:
        std::vector<std::unique_ptr<Command>> commands;
    };
  }
}
