#include "handlers.hpp"

#include <exception>
#include <optional>
#include <pqxx/pqxx>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/request.hpp"
#include "commands/request_util.hpp"
#include "irc/message.hpp"
#include "localization/line_id.hpp"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle,
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      pqxx::connection &conn) {
    std::optional<command::Request> request =
        command::generate_request(command_loader, message, conn);

    if (request.has_value()) {
      try {
        auto response = command_loader.run(bundle, request.value());

        if (response.has_value()) {
          try {
            auto str = std::get<std::string>(*response);
            bundle.irc_client.say(message.source.login, str);
          } catch (const std::exception &e) {
          }

          try {
            auto strs = std::get<std::vector<std::string>>(*response);
            for (const std::string &str : strs) {
              bundle.irc_client.say(message.source.login, str);
            }
          } catch (const std::exception &e) {
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
  }
}
