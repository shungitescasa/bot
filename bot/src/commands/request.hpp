#pragma once

#include <memory>
#include <optional>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <string>

#include "../irc/message.hpp"
#include "../schemas/channel.hpp"
#include "../schemas/user.hpp"

namespace bot::command {
  struct Request;
}

#include "commands/command.hpp"
#include "database.hpp"

namespace bot::command {
  struct Requester {
      schemas::Channel channel;
      schemas::ChannelPreferences channel_preferences;
      schemas::User user;
      schemas::UserRights user_rights;
  };

  struct Request {
      std::string command_id;
      std::optional<std::string> subcommand_id;
      std::optional<std::string> message;
      const irc::Message<irc::MessageType::Privmsg> &irc_message;

      const Requester requester;

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;
  };

  std::optional<Request> generate_request(
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      const Requester &requester, std::unique_ptr<db::BaseDatabase> &conn);

  std::optional<Requester> get_requester(
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      std::unique_ptr<db::BaseDatabase> &conn);
}
