#include "handlers.hpp"

#include <exception>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/request.hpp"
#include "commands/request_util.hpp"
#include "irc/message.hpp"
#include "localization/line_id.hpp"
#include "logger.hpp"
#include "utils/string.hpp"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle,
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      pqxx::connection &conn) {
    if (utils::string::string_contains_sql_injection(message.message)) {
      log::warn("PrivateMessageHandler",
                "Received the message in #" + message.source.login +
                    " with SQL injection: " + message.message);
      return;
    }

    std::optional<command::Request> request =
        command::generate_request(command_loader, message, conn);

    if (request.has_value()) {
      try {
        auto response = command_loader.run(bundle, request.value());

        if (response.has_value()) {
          if (response->is_single()) {
            bundle.irc_client.say(message.source.login, response->get_single());
          } else if (response->is_multiple()) {
            for (const std::string &msg : response->get_multiple()) {
              bundle.irc_client.say(message.source.login, msg);
            }
          }
        }
      } catch (const std::exception &e) {
        std::string line =
            bundle.localization
                .get_formatted_line(request.value(), loc::LineId::ErrorTemplate,
                                    {e.what()})
                .value();

        bundle.irc_client.say(message.source.login, line);
      }
    }

    pqxx::work work(conn);
    pqxx::result channels =
        work.exec("SELECT id FROM channels WHERE alias_id = " +
                  std::to_string(message.source.id));

    if (!channels.empty()) {
      int channel_id = channels[0][0].as<int>();
      pqxx::result cmds =
          work.exec("SELECT message FROM custom_commands WHERE name = '" +
                    message.message + "' AND channel_id = '" +
                    std::to_string(channel_id) + "'");

      if (!cmds.empty()) {
        std::string msg = cmds[0][0].as<std::string>();

        bundle.irc_client.say(message.source.login, msg);
      }
    }

    work.commit();
  }
}
