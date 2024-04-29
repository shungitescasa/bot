#pragma once

#include "bundle.hpp"
#include "commands/command.hpp"
#include "irc/message.hpp"
#include "pqxx/pqxx"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle,
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      const pqxx::work &work);
}
