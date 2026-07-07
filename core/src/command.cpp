#include "core/command.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace bot {
  Response::Response() {
    this->single = std::nullopt;
    this->multiple = std::nullopt;
  }

  Response::Response(std::string single) {
    this->single = single;
    this->multiple = std::nullopt;
  }

  Response::Response(std::vector<std::string> multiple) {
    this->single = std::nullopt;
    this->multiple = multiple;
  }

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

  void CommandLoader::add(CommandBox command) {
    auto it = std::find_if(
        this->commands.begin(), this->commands.end(),
        [&](const auto &x) { return command->get_name() == x->get_name(); });
    if (it != this->commands.end()) {
      this->commands.erase(it);
    }
    this->commands.push_back(command);
  }

  const Response CommandLoader::run(const Request &request) const {
    auto command = std::find_if(
        this->commands.begin(), this->commands.end(), [&](const auto &x) {
          auto aliases = x->get_aliases();
          return x->get_name() == request.command_id ||
                 std::any_of(aliases.begin(), aliases.end(),
                             [&](const std::string &alias) {
                               return alias == request.command_id;
                             });
        });

    if (command == this->commands.end()) return Response{};

    return command->get()->run(request);
  }

  CommandVec &CommandLoader::get_commands() { return this->commands; }

  Requester::Requester(const Message<MessageType::ChatMessage> &message) {
    this->sender = message.sender;
    this->source = message.source;
  }

  std::optional<Request> Request::create(
      const CommandVec &commands,
      const Message<MessageType::ChatMessage> &message,
      const Requester &requester) {
    std::string contents = message.contents;
    if (contents.empty()) return std::nullopt;

    auto parts = std::ranges::views::split(contents, ' ');

    std::string command_id(parts.front().begin(), parts.front().end());

    auto cmd = std::find_if(
        commands.begin(), commands.end(), [&command_id](const auto &c) {
          auto aliases = c->get_aliases();
          return c->get_name() == command_id ||
                 std::any_of(aliases.begin(), aliases.end(),
                             [&command_id](const std::string &alias) {
                               return alias == command_id;
                             });
        });

    if (cmd == commands.end()) return std::nullopt;

    Request r{.requester = requester};
    r.command_id = command_id;

    return r;
  }
}