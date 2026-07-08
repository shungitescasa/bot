#include "core/command.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/data/chat.hpp"
#include "core/utils.hpp"

namespace bot {
  const std::string Response::get_single() const {
    return this->single.value();
  }

  const std::vector<std::string> Response::get_multiple() const {
    return this->multiple.value();
  }

  const bool Response::is_single() const { return this->single.has_value(); }

  const bool Response::is_multiple() const {
    return this->multiple.has_value();
  }

  const bool Response::is_empty() const {
    return !this->single.has_value() && !this->multiple.has_value();
  }

  const CommandData Command::data() const {
    return {this->name, this->delay_seconds, this->aliases, this->subcommands};
  }

  void CommandLoader::add(CommandBox command) {
    auto it = std::find_if(
        this->commands.begin(), this->commands.end(),
        [&](const auto &x) { return command->data().name == x->data().name; });
    if (it != this->commands.end()) {
      this->commands.erase(it);
    }
    this->commands.push_back(command);
  }

  bool CommandLoader::has(const std::string &command_id) const {
    return std::any_of(
        this->commands.begin(), this->commands.end(),
        [&](const CommandBox &x) { return x->data().name == command_id; });
  }

  const Response CommandLoader::run(const Request &request) const {
    auto command = std::find_if(
        this->commands.begin(), this->commands.end(), [&](const CommandBox &x) {
          auto aliases = x->data().aliases;
          return x->data().name == request.command_id ||
                 std::any_of(aliases.begin(), aliases.end(),
                             [&](const std::string &alias) {
                               return alias == request.command_id;
                             });
        });

    if (command == this->commands.end()) return Response{};

    return command->get()->run(request);
  }

  CommandVec &CommandLoader::get_commands() { return this->commands; }

  Requester::Requester(const Message<MessageType::ChatMessage> &message,
                       std::unique_ptr<data::BaseDatabase> &conn) {
    Configuration &cfg = Configuration::get_instance();

    // fetching room
    std::vector<data::Room> rooms = conn->query_all<data::Room>(
        "SELECT * FROM rooms WHERE (name != '' AND name = $1) OR (alias_id != "
        "-1 AND alias_id = $2) LIMIT 1",
        {message.source.normalize(), std::to_string(message.source.id)});

    if (rooms.empty()) {
      conn->exec(
          "INSERT INTO rooms(name, alias_id) VALUES ($1, $2)",
          {message.source.normalize(), std::to_string(message.source.id)});

      rooms = conn->query_all<data::Room>(
          "SELECT * FROM rooms WHERE (name != '' AND name = $1) OR (alias_id "
          "!= "
          "-1 AND alias_id = $2) LIMIT 1",
          {message.source.normalize(), std::to_string(message.source.id)});
    }

    room = rooms.front();

    // updating room name
    if (room.name != message.source.normalize()) {
      conn->exec("UPDATE rooms SET name = $1 WHERE id = $2",
                 {message.source.normalize(), std::to_string(room.id)});
      room.name = message.source.normalize();
    }

    // fetching room preference
    std::vector<data::RoomPreferences> prefs =
        conn->query_all<data::RoomPreferences>(
            "SELECT * FROM room_preferences WHERE id = $1 LIMIT 1",
            {std::to_string(room.id)});

    if (prefs.empty()) {
      conn->exec(
          "INSERT INTO room_preferences(id, prefix, locale) VALUES ($1, "
          "$2, $3)",
          {std::to_string(room.id), DEFAULT_PREFIX, DEFAULT_LOCALE_ID});

      prefs = conn->query_all<data::RoomPreferences>(
          "SELECT * FROM room_preferences WHERE id = $1",
          {std::to_string(room.id)});
    }

    room_preferences = prefs.front();

    // fetching sender
    std::vector<data::Sender> senders = conn->query_all<data::Sender>(
        "SELECT * FROM senders WHERE (name != '' AND name = $1) OR (alias_id "
        "!= -1 AND alias_id = $2) LIMIT 1",
        {message.sender.login, std::to_string(message.sender.id)});

    if (senders.empty()) {
      conn->exec("INSERT INTO senders(name, alias_id) VALUES ($1, $2)",
                 {message.sender.login, std::to_string(message.sender.id)});

      senders = conn->query_all<data::Sender>(
          "SELECT * FROM senders WHERE (name != '' AND name = $1) OR (alias_id "
          "!= -1 AND alias_id = $2) LIMIT 1",
          {message.sender.login, std::to_string(message.sender.id)});
    }

    sender = senders.front();

    // updating sender name
    if (sender.name != message.sender.login) {
      conn->exec("UPDATE senders SET name = $1 WHERE id = $2",
                 {message.sender.login, std::to_string(sender.id)});
      sender.name = message.sender.login;
    }

    // -- setting permissions
    data::PermissionLevel level = data::PermissionLevel::User;
    const auto &badges = message.sender.badges;

    if (sender.name == room.name) {
      level = data::PermissionLevel::Broadcaster;
    } else if (badges.contains("moderator") ||
               badges.contains("lead_moderator")) {
      level = data::PermissionLevel::Moderator;
    } else if (badges.contains("vip")) {
      level = data::PermissionLevel::VIP;
    }

    std::vector<data::SenderRights> sender_rights =
        conn->query_all<data::SenderRights>(
            "SELECT * FROM sender_rights WHERE sender_id = $1 AND room_id = $2 "
            "LIMIT 1",
            {std::to_string(sender.id), std::to_string(room.id)});

    if (sender_rights.empty()) {
      conn->exec(
          "INSERT INTO sender_rights(sender_id, room_id, level) VALUES ($1, "
          "$2, $3)",
          {std::to_string(sender.id), std::to_string(room.id),
           std::to_string((int)level)});

      sender_rights = conn->query_all<data::SenderRights>(
          "SELECT * FROM sender_rights WHERE sender_id = $1 AND room_id = $2 "
          "LIMIT 1",
          {std::to_string(sender.id), std::to_string(room.id)});
    }

    sender_right = sender_rights.front();

    if (sender_right.level != static_cast<int>(level)) {
      conn->exec("UPDATE sender_rights SET level = $1 WHERE id = $2",
                 {std::to_string(static_cast<int>(level)),
                  std::to_string(sender_right.id)});

      sender_right.level = static_cast<int>(level);
    }
  }

  sol::table Request::as_lua_table(std::shared_ptr<sol::state> state) const {
    sol::table o = state->create_table();

    o["command_id"] = command_id;
    if (this->subcommand_id.has_value()) {
      o["subcommand_id"] = subcommand_id.value();
    } else {
      o["subcommand_id"] = sol::lua_nil;
    }
    if (contents.has_value()) {
      o["message"] = contents.value();
    } else {
      o["message"] = sol::lua_nil;
    }

    if (reply.has_value()) {
      sol::table r = state->create_table();
      r["id"] = reply->id;
      r["login"] = reply->login;
      r["display_name"] = reply->display_name;
      r["user_id"] = reply->user_id;
      r["message"] = reply->message;
      o["reply"] = r;
    } else {
      o["reply"] = sol::lua_nil;
    }

    o["sender"] = requester.sender.as_lua_table(state);
    o["channel"] = requester.room.as_lua_table(state);
    o["room"] = requester.room.as_lua_table(state);
    o["channel_preference"] = requester.room_preferences.as_lua_table(state);
    o["room_preference"] = requester.room_preferences.as_lua_table(state);
    o["rights"] = requester.sender_right.as_lua_table(state);

    return o;
  }

  std::optional<Request> Request::create(
      const CommandDataVec &commands,
      const Message<MessageType::ChatMessage> &message,
      const Requester &requester) {
    std::string contents = message.contents;
    if (contents.empty() ||
        !contents.starts_with(requester.room_preferences.prefix) ||
        requester.room_preferences.silent_mode ||
        requester.room.parted_at.has_value() ||
        requester.sender_right.level ==
            static_cast<int>(data::PermissionLevel::Suspended))
      return std::nullopt;

    contents = contents.substr(requester.room_preferences.prefix.length());

    auto parts = utils::string::split_and_collect(contents, ' ');

    std::string command_id = parts.front();

    auto cmd = std::find_if(
        commands.begin(), commands.end(), [&command_id](const CommandData &c) {
          auto aliases = c.aliases;
          return c.name == command_id ||
                 std::any_of(aliases.begin(), aliases.end(),
                             [&command_id](const std::string &alias) {
                               return alias == command_id;
                             });
        });

    if (cmd == commands.end()) return std::nullopt;

    Request r{.command_id = command_id,
              .reply = message.reply,
              .requester = requester};

    parts.erase(parts.begin());
    if (parts.empty()) return r;

    if (std::any_of(cmd->subcommands.begin(), cmd->subcommands.end(),
                    [&](const std::string &x) {
                      return x == "*" || x == parts.front();
                    })) {
      r.subcommand_id = parts.front();
      parts.erase(parts.begin());
    }

    std::string rcontents = utils::string::join(parts, " ");
    if (!rcontents.empty()) r.contents = rcontents;

    return r;
  }
}