#pragma once

#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "../irc/message.hpp"
#include "../schemas/channel.hpp"
#include "../schemas/user.hpp"

namespace bot::command {
  struct Request {
      std::string command_id;
      std::optional<std::string> subcommand_id;
      std::optional<std::string> message;
      const irc::Message<irc::MessageType::Privmsg> &irc_message;

      schemas::Channel channel;
      schemas::User user;

      pqxx::connection &conn;
  };
}
