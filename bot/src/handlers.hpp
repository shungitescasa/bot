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
      pqxx::connection &conn);

  void make_markov_response(
      const InstanceBundle &bundle,
      const irc::Message<irc::MessageType::Privmsg> &message,
      const schemas::Channel &channel,
      const schemas::ChannelPreferences &preference);
}
