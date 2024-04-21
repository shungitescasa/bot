#pragma once

#include <optional>
#include <string>

#include "../irc/message.hpp"
#include "command.hpp"

namespace bot {
  namespace command {
    class Request {
      public:
        Request(const command::CommandLoader &command_loader,
                const irc::Message<irc::MessageType::Privmsg> &irc_message)
            : irc_message(irc_message), command_loader(command_loader){};
        ~Request() = default;

        bool fill_request();

        const std::string &get_command_id() const { return this->command_id; };
        const std::optional<std::string> &get_subcommand_id() const {
          return this->subcommand_id;
        };
        const std::optional<std::string> &get_message() const {
          return this->message;
        };

        const irc::Message<irc::MessageType::Privmsg> &get_irc_message() const {
          return this->irc_message;
        };

      private:
        std::string command_id;
        std::optional<std::string> subcommand_id;
        std::optional<std::string> message;

        const irc::Message<irc::MessageType::Privmsg> &irc_message;
        const command::CommandLoader &command_loader;
    };

  }

}
