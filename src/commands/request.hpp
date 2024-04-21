#pragma once

#include <optional>
#include <string>

#include "../irc/message.hpp"

namespace bot::command {
  struct Request {
      std::string command_id;
      std::optional<std::string> subcommand_id;
      std::optional<std::string> message;
      const irc::Message<irc::MessageType::Privmsg> &irc_message;
  };
}
