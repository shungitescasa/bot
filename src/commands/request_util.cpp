#include "request_util.hpp"

#include <algorithm>
#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "../constants.hpp"
#include "../irc/message.hpp"
#include "../schemas/channel.hpp"
#include "command.hpp"
#include "request.hpp"

namespace bot::command {
  std::optional<Request> generate_request(
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      pqxx::connection &conn) {
    pqxx::work *work;

    work = new pqxx::work(conn);

    std::vector<std::string> parts =
        utils::string::split_text(irc_message.message, ' ');

    std::string command_id = parts[0];

    if (command_id.substr(0, DEFAULT_PREFIX.length()) != DEFAULT_PREFIX) {
      return std::nullopt;
    }

    command_id =
        command_id.substr(DEFAULT_PREFIX.length(), command_id.length());

    bool found = std::any_of(
        command_loader.get_commands().begin(),
        command_loader.get_commands().end(),
        [&](const auto &command) { return command->get_name() == command_id; });

    if (!found) {
      return std::nullopt;
    }

    parts.erase(parts.begin());

    pqxx::result query = work->exec("SELECT * FROM channels WHERE alias_id = " +
                                    std::to_string(irc_message.source.id));

    // Create new channel data in the database if it didn't exist b4
    if (query.empty()) {
      work->exec("INSERT INTO channels (alias_id, alias_name) VALUES (" +
                 std::to_string(irc_message.source.id) + ", '" +
                 irc_message.source.login + "')");

      work->commit();

      delete work;
      work = new pqxx::work(conn);

      query = work->exec("SELECT * FROM channels WHERE alias_id = " +
                         std::to_string(irc_message.source.id));
    }

    schemas::Channel channel(query[0]);

    query = work->exec("SELECT * FROM channel_preferences WHERE channel_id = " +
                       std::to_string(channel.get_id()));

    // Create new channel preference data in the database if it didn't exist b4
    if (query.empty()) {
      work->exec(
          "INSERT INTO channel_preferences (channel_id, prefix, locale) VALUES "
          "(" +
          std::to_string(channel.get_id()) + ", '" + DEFAULT_PREFIX + "', '" +
          DEFAULT_LOCALE_ID + "')");

      work->commit();

      delete work;
      work = new pqxx::work(conn);

      query =
          work->exec("SELECT * FROM channel_preferences WHERE channel_id = " +
                     std::to_string(channel.get_id()));
    }

    schemas::ChannelPreferences channel_preferences(query[0]);

    query = work->exec("SELECT * FROM users WHERE alias_id = " +
                       std::to_string(irc_message.sender.id));

    // Create new user data in the database if it didn't exist before
    if (query.empty()) {
      work->exec("INSERT INTO users (alias_id, alias_name) VALUES (" +
                 std::to_string(irc_message.sender.id) + ", '" +
                 irc_message.sender.login + "')");

      work->commit();

      delete work;
      work = new pqxx::work(conn);

      query = work->exec("SELECT * FROM users WHERE alias_id = " +
                         std::to_string(irc_message.sender.id));
    }

    schemas::User user(query[0]);

    if (user.get_alias_name() != irc_message.sender.login) {
      work->exec("UPDATE users SET alias_name = '" + irc_message.sender.login +
                 "' WHERE id = " + std::to_string(user.get_id()));
      work->commit();

      delete work;
      work = new pqxx::work(conn);

      user.set_alias_name(irc_message.sender.login);
    }

    schemas::PermissionLevel level = schemas::PermissionLevel::USER;
    const auto &badges = irc_message.sender.badges;

    if (user.get_alias_id() == channel.get_alias_id()) {
      level = schemas::PermissionLevel::BROADCASTER;
    } else if (std::any_of(badges.begin(), badges.end(), [&](const auto &x) {
                 return x.first == "moderator";
               })) {
      level = schemas::PermissionLevel::MODERATOR;
    } else if (std::any_of(badges.begin(), badges.end(),
                           [&](const auto &x) { return x.first == "vip"; })) {
      level = schemas::PermissionLevel::VIP;
    }

    query = work->exec("SELECT * FROM user_rights WHERE user_id = " +
                       std::to_string(user.get_id()) +
                       " AND channel_id = " + std::to_string(channel.get_id()));

    if (query.empty()) {
      work->exec(
          "INSERT INTO user_rights (user_id, channel_id, level) VALUES (" +
          std::to_string(user.get_id()) + ", " +
          std::to_string(channel.get_id()) + ", " + std::to_string(level) +
          ")");

      work->commit();

      delete work;
      work = new pqxx::work(conn);

      query = work->exec("SELECT * FROM user_rights WHERE user_id = " +
                         std::to_string(user.get_id()) + " AND channel_id = " +
                         std::to_string(channel.get_id()));
    }

    schemas::UserRights user_rights(query[0]);

    if (user_rights.get_level() != level) {
      work->exec("UPDATE user_rights SET level = " + std::to_string(level) +
                 " WHERE id = " + std::to_string(query[0][0].as<int>()));

      work->commit();

      user_rights.set_level(level);
    }

    delete work;

    if (parts.empty()) {
      Request req{command_id,  std::nullopt, std::nullopt,
                  irc_message, channel,      channel_preferences,
                  user,        user_rights,  conn};

      return req;
    }

    std::optional<std::string> subcommand_id = parts[0];
    if (subcommand_id->empty()) {
      subcommand_id = std::nullopt;
    }
    parts.erase(parts.begin());

    std::optional<std::string> message = utils::string::join_vector(parts, ' ');

    if (message->empty()) {
      message = std::nullopt;
    }

    Request req{command_id,  subcommand_id, message,
                irc_message, channel,       channel_preferences,
                user,        user_rights,   conn};
    return req;
  }
}
