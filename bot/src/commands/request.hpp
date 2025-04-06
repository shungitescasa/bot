#pragma once

#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <sol/state.hpp>
#include <sol/table.hpp>
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
      schemas::ChannelPreferences channel_preferences;
      schemas::User user;
      schemas::UserRights user_rights;

      pqxx::connection &conn;

      sol::table as_lua_table(std::shared_ptr<sol::state> luaState) const;
  };
}
