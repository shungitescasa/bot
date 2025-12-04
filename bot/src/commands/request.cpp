#include "commands/request.hpp"

#include <algorithm>
#include <optional>
#include <sol/types.hpp>
#include <string>
#include <vector>

#include "config.hpp"
#include "constants.hpp"
#include "database.hpp"
#include "schemas/channel.hpp"
#include "schemas/user.hpp"
#include "utils/string.hpp"

namespace bot::command {
  sol::table Request::as_lua_table(std::shared_ptr<sol::state> luaState) const {
    sol::table o = luaState->create_table();

    o["command_id"] = this->command_id;
    if (this->subcommand_id.has_value()) {
      o["subcommand_id"] = this->subcommand_id.value();
    } else {
      o["subcommand_id"] = sol::lua_nil;
    }
    if (this->message.has_value()) {
      o["message"] = this->message.value();
    } else {
      o["message"] = sol::lua_nil;
    }

    o["sender"] = requester.user.as_lua_table(luaState);
    o["channel"] = requester.channel.as_lua_table(luaState);
    o["channel_preference"] =
        requester.channel_preferences.as_lua_table(luaState);
    o["rights"] = requester.user_rights.as_lua_table(luaState);

    return o;
  }

  std::optional<Requester> get_requester(
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      std::unique_ptr<db::BaseDatabase> &conn, const Configuration &cfg) {
    // fetching channel
    std::vector<schemas::Channel> chans = conn->query_all<schemas::Channel>(
        "SELECT * FROM channels WHERE alias_id = $1",
        {std::to_string(irc_message.source.id)});

    if (chans.empty()) {
      conn->exec(
          "INSERT INTO channels(alias_id, alias_name) VALUES ($1, $2)",
          {std::to_string(irc_message.source.id), irc_message.source.login});

      chans = conn->query_all<schemas::Channel>(
          "SELECT * FROM channels WHERE alias_id = $1",
          {std::to_string(irc_message.source.id)});
    }

    schemas::Channel channel = chans[0];
    if (channel.get_opted_out_at().has_value()) {
      return std::nullopt;
    }

    // fetching channel preference
    std::vector<schemas::ChannelPreferences> prefs =
        conn->query_all<schemas::ChannelPreferences>(
            "SELECT * FROM channel_preferences WHERE id = $1",
            {std::to_string(channel.get_id())});

    if (prefs.empty()) {
      conn->exec(
          "INSERT INTO channel_preferences(id, prefix, locale) VALUES ($1, "
          "$2, $3)",
          {std::to_string(channel.get_id()), DEFAULT_PREFIX,
           DEFAULT_LOCALE_ID});

      prefs = conn->query_all<schemas::ChannelPreferences>(
          "SELECT * FROM channel_preferences WHERE id = $1",
          {std::to_string(channel.get_id())});
    }

    schemas::ChannelPreferences pref = prefs[0];

    // fetching channel preference
    std::vector<schemas::User> users = conn->query_all<schemas::User>(
        "SELECT * FROM users WHERE alias_id = $1",
        {std::to_string(irc_message.sender.id)});

    if (users.empty()) {
      conn->exec(
          "INSERT INTO users(alias_id, alias_name) VALUES ($1, "
          "$2)",
          {std::to_string(irc_message.sender.id), irc_message.sender.login});

      users = conn->query_all<schemas::User>(
          "SELECT * FROM users WHERE alias_id = $1",
          {std::to_string(irc_message.sender.id)});
    }

    schemas::User user = users[0];

    // updating username
    if (user.get_alias_name() != irc_message.sender.login) {
      conn->exec("UPDATE users SET alias_name = $1 WHERE id = $2",
                 {irc_message.sender.login, std::to_string(user.get_id())});

      user.set_alias_name(irc_message.sender.login);
    }

    // setting permissions
    schemas::PermissionLevel level = schemas::PermissionLevel::USER;
    const auto &badges = irc_message.sender.badges;

    if (std::any_of(
            cfg.twitch.superuser_ids.begin(), cfg.twitch.superuser_ids.end(),
            [&user](const int &x) { return x == user.get_alias_id(); })) {
      level = schemas::PermissionLevel::SUPERUSER;
    } else if (std::any_of(cfg.twitch.trusted_user_ids.begin(),
                           cfg.twitch.trusted_user_ids.end(),
                           [&user](const int &x) {
                             return x == user.get_alias_id();
                           })) {
      level = schemas::PermissionLevel::TRUSTED;
    } else if (user.get_alias_id() == channel.get_alias_id()) {
      level = schemas::PermissionLevel::BROADCASTER;
    } else if (std::any_of(badges.begin(), badges.end(), [&](const auto &x) {
                 return x.first == "moderator";
               })) {
      level = schemas::PermissionLevel::MODERATOR;
    } else if (std::any_of(badges.begin(), badges.end(),
                           [&](const auto &x) { return x.first == "vip"; })) {
      level = schemas::PermissionLevel::VIP;
    }

    std::vector<schemas::UserRights> user_rights =
        conn->query_all<schemas::UserRights>(
            "SELECT * FROM user_rights WHERE user_id = $1 AND channel_id = $2",
            {std::to_string(user.get_id()), std::to_string(channel.get_id())});

    if (user_rights.empty()) {
      conn->exec(
          "INSERT INTO user_rights(user_id, channel_id, level) VALUES ($1, "
          "$2, $3)",
          {std::to_string(user.get_id()), std::to_string(channel.get_id()),
           std::to_string(level)});

      user_rights = conn->query_all<schemas::UserRights>(
          "SELECT * FROM user_rights WHERE user_id = $1 AND channel_id = $2",
          {std::to_string(user.get_id()), std::to_string(channel.get_id())});
    }

    schemas::UserRights user_right = user_rights[0];

    if (user_right.get_level() != level) {
      conn->exec("UPDATE user_rights SET level = $1 WHERE id = $2",
                 {std::to_string(level), std::to_string(user_right.get_id())});

      user_right.set_level(level);
    }

    return (Requester){channel, pref, user, user_right};
  }

  std::optional<Request> generate_request(
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      const Requester &requester, std::unique_ptr<db::BaseDatabase> &conn) {
    // --- FETCHING MESSAGES
    std::string fullmsg = irc_message.message;
    const std::string &prefix = requester.channel_preferences.get_prefix();

    if (fullmsg.empty() || fullmsg.substr(0, prefix.length()) != prefix ||
        std::any_of(requester.channel_preferences.get_features().begin(),
                    requester.channel_preferences.get_features().end(),
                    [](const schemas::ChannelFeature &f) {
                      return f == schemas::ChannelFeature::SILENT_MODE;
                    })) {
      return std::nullopt;
    }

    fullmsg = fullmsg.substr(prefix.length());

    std::vector<std::string> parts = utils::string::split_text(fullmsg, ' ');

    std::string command_id = parts[0];

    auto &cmds = command_loader.get_commands();
    auto cmd =
        std::find_if(cmds.begin(), cmds.end(), [&command_id](const auto &c) {
          auto aliases = c->get_aliases();
          return c->get_name() == command_id ||
                 std::any_of(aliases.begin(), aliases.end(),
                             [&command_id](const std::string &alias) {
                               return alias == command_id;
                             });
        });

    if (cmd == cmds.end()) {
      return std::nullopt;
    }

    parts.erase(parts.begin());

    command_id = (*cmd)->get_name();

    Request req{command_id, std::nullopt, std::nullopt, irc_message, requester};

    if (parts.empty()) {
      return req;
    }

    std::optional<std::string> scid = parts[0];
    auto scids = (*cmd)->get_subcommand_ids();

    if (std::any_of(scids.begin(), scids.end(),
                    [&](const auto &x) { return x == scid.value(); })) {
      parts.erase(parts.begin());
    } else {
      scid = std::nullopt;
    }

    req.subcommand_id = scid;

    std::optional<std::string> message = utils::string::join_vector(parts, ' ');
    req.message = message;

    return req;
  }
}