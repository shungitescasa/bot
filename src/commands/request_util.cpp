#include "request_util.hpp"

#include <optional>
#include <pqxx/pqxx>

#include "../constants.hpp"
#include "../irc/message.hpp"
#include "command.hpp"
#include "request.hpp"

namespace bot::command {
  std::optional<Request> generate_request(
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &irc_message,
      const pqxx::work &work) {
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

    if (parts.empty()) {
      Request req{command_id, std::nullopt, std::nullopt, irc_message, work};

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

    Request req{command_id, subcommand_id, message, irc_message, work};
    return req;
  }
}
