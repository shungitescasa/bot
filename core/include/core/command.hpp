#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/message.hpp"
#include "rpc/msgpack.hpp"

#define ADD_COMMAND(loader, name) \
  loader.add(std::make_unique<bot::builtin::name>());

namespace bot {
  class Command;

  using CommandBox = std::shared_ptr<Command>;
  using CommandVec = std::vector<CommandBox>;

  struct Requester {
      MessageSender sender;
      MessageSource source;

      MSGPACK_DEFINE(sender, source);

      Requester() = default;
      Requester(const Message<MessageType::ChatMessage> &message);
  };

  struct Request {
      std::string command_id = "";
      std::optional<std::string> subcommand_id = std::nullopt,
                                 contents = std::nullopt;
      Requester requester;

      MSGPACK_DEFINE(command_id, subcommand_id, contents, requester);

      static std::optional<Request> create(
          const CommandVec &commands,
          const Message<MessageType::ChatMessage> &message,
          const Requester &requester);
  };

  class Response {
    public:
      Response() = default;
      Response(std::string single) : single(single), multiple(std::nullopt) {}
      Response(std::vector<std::string> multiple)
          : single(std::nullopt), multiple(multiple) {}

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

  class Command {
    public:
      ~Command() = default;

      virtual const std::string get_name() const = 0;
      virtual const Response run(const Request &request) const = 0;

      std::vector<std::string> get_aliases() const { return {}; }
      int get_delay_seconds() const { return 5; }
      std::vector<std::string> get_subcommands() const { return {}; }
  };

  class CommandLoader {
    public:
      CommandLoader() = default;
      ~CommandLoader() = default;

      void add(CommandBox command);
      CommandVec &get_commands();

      const Response run(const Request &request) const;

    private:
      CommandVec commands;
  };
}