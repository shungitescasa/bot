#include "handlers.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <variant>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/request.hpp"
#include "commands/request_util.hpp"
#include "irc/message.hpp"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle,
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      const pqxx::work &work) {
    std::optional<command::Request> request =
        command::generate_request(command_loader, message);

    if (request.has_value()) {
      auto o_response = command_loader.run(bundle, request.value());

      if (o_response.has_value()) {
        auto response = o_response.value();

        try {
          auto str = std::get<std::string>(response);
          bundle.irc_client.say(message.source.login, str);
        } catch (const std::exception &e) {
        }

        try {
          auto strs = std::get<std::vector<std::string>>(response);
          for (const std::string &str : strs) {
            bundle.irc_client.say(message.source.login, str);
          }
        } catch (const std::exception &e) {
        }
      }
    }
  }
}
