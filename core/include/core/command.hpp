#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/data/chat.hpp"
#include "core/message.hpp"
#include "rpc/msgpack.hpp"

#define ADD_COMMAND(loader, name) \
  loader.add(std::make_unique<bot::builtin::name>());

namespace bot {
  class Command;
  struct CommandData;

  using CommandBox = std::shared_ptr<Command>;
  using CommandVec = std::vector<CommandBox>;
  using CommandDataVec = std::vector<CommandData>;

  struct Requester {
      data::Room room;
      data::RoomPreferences room_preferences;
      data::Sender sender;
      data::SenderRights sender_right;

      MSGPACK_DEFINE(room, room_preferences, sender, sender_right);

      Requester() = default;
      Requester(const Message<MessageType::ChatMessage> &message,
                std::unique_ptr<data::BaseDatabase> &conn);
  };

  struct Request {
      std::string command_id = "";
      std::optional<std::string> subcommand_id = std::nullopt,
                                 contents = std::nullopt;
      std::optional<MessageReply> reply = std::nullopt;
      std::unordered_map<std::string, std::string> meta = {};
      Requester requester;

      MSGPACK_DEFINE(command_id, subcommand_id, contents, requester, reply,
                     meta);

      static std::optional<Request> create(
          const CommandDataVec &commands,
          const Message<MessageType::ChatMessage> &message,
          const Requester &requester);

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const;
  };

  class Response {
    public:
      Response() = default;
      Response(std::string single) : single(single), multiple(std::nullopt) {}
      Response(std::vector<std::string> multiple)
          : single(std::nullopt), multiple(multiple) {}
      Response(std::runtime_error e) : single(std::format("⁉️ {}", e.what())) {}

      const std::string get_single() const;
      const std::vector<std::string> get_multiple() const;

      const bool is_single() const;
      const bool is_multiple() const;
      const bool is_empty() const;

      MSGPACK_DEFINE(single, multiple);

    private:
      std::optional<std::string> single;
      std::optional<std::vector<std::string>> multiple;
  };

  struct CommandData {
      std::string name;
      int delay_seconds;
      std::vector<std::string> aliases, subcommands;

      MSGPACK_DEFINE(name, delay_seconds, aliases, subcommands);
  };

  class Command {
    public:
      explicit Command(std::string name, int delay_seconds = 5,
                       std::vector<std::string> aliases = {},
                       std::vector<std::string> subcommands = {})
          : name(std::move(name)),
            delay_seconds(delay_seconds),
            aliases(std::move(aliases)),
            subcommands(std::move(subcommands)) {};

      ~Command() = default;

      virtual const Response run(const Request &request) const = 0;

      const CommandData data() const;

    protected:
      std::string name;
      int delay_seconds;
      std::vector<std::string> aliases, subcommands;
  };

  class CommandLoader {
    public:
      CommandLoader() = default;
      ~CommandLoader() = default;

      void add(CommandBox command);
      bool has(const std::string &command_id) const;
      CommandVec &get_commands();

      const Response run(const Request &request) const;

    protected:
      CommandVec commands;
  };
}
