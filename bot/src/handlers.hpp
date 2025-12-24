#pragma once

#include "bundle.hpp"
#include "commands/command.hpp"
#include "irc/message.hpp"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle, command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message);

  void handle_timers(chat::ChatClient *irc_client,
                     Configuration *configuration);
}
