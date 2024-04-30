#include "request_util.hpp"

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

    pqxx::result channel_query =
        work->exec("SELECT * FROM channels WHERE alias_id = " +
                   std::to_string(irc_message.source.id));

    // Create new channel data in the database if it didn't exist b4
    if (channel_query.empty()) {
      work->exec("INSERT INTO channels (alias_id, alias_name) VALUES (" +
                 std::to_string(irc_message.source.id) + ", '" +
                 irc_message.source.login + "')");

      work->commit();

      delete work;
      work = new pqxx::work(conn);

      channel_query = work->exec("SELECT * FROM channels WHERE alias_id = " +
                                 std::to_string(irc_message.source.id));
    }

    schemas::Channel channel(channel_query[0]);

    delete work;

    if (parts.empty()) {
      Request req{command_id,  std::nullopt, std::nullopt,
                  irc_message, channel,      conn};

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

    Request req{command_id, subcommand_id, message, irc_message, channel, conn};
    return req;
  }
}
