#include "command.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <string>

#include "../bundle.hpp"
#include "../modules/event.hpp"
#include "../modules/join.hpp"
#include "../modules/massping.hpp"
#include "../modules/notify.hpp"
#include "../modules/ping.hpp"
#include "../utils/chrono.hpp"
#include "request.hpp"

namespace bot {
  namespace command {
    CommandLoader::CommandLoader() {
      this->add_command(std::make_unique<mod::Ping>());
      this->add_command(std::make_unique<mod::Massping>());
      this->add_command(std::make_unique<mod::Event>());
      this->add_command(std::make_unique<mod::Notify>());
      this->add_command(std::make_unique<mod::Join>());
    }

    void CommandLoader::add_command(std::unique_ptr<Command> command) {
      this->commands.push_back(std::move(command));
    }

    std::optional<std::variant<std::vector<std::string>, std::string>>
    CommandLoader::run(const InstanceBundle &bundle,
                       const Request &request) const {
      auto command = std::find_if(
          this->commands.begin(), this->commands.end(),
          [&](const auto &x) { return x->get_name() == request.command_id; });

      if (command == this->commands.end()) {
        return std::nullopt;
      }

      if ((*command)->get_permission_level() >
          request.user_rights.get_level()) {
        return std::nullopt;
      }

      pqxx::work work(request.conn);

      pqxx::result action_query = work.exec(
          "SELECT sent_at FROM actions WHERE user_id = " +
          std::to_string(request.user.get_id()) +
          " AND channel_id = " + std::to_string(request.channel.get_id()) +
          " AND command = '" + request.command_id + "' ORDER BY sent_at DESC");

      if (!action_query.empty()) {
        auto last_sent_at = utils::chrono::string_to_time_point(
            action_query[0][0].as<std::string>());

        auto now = std::chrono::system_clock::now();
        auto now_time_it = std::chrono::system_clock::to_time_t(now);
        auto now_tm = std::gmtime(&now_time_it);
        now = std::chrono::system_clock::from_time_t(std::mktime(now_tm));

        auto difference = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_sent_at);

        if (difference.count() < command->get()->get_delay_seconds()) {
          return std::nullopt;
        }
      }

      std::string arguments;

      if (request.subcommand_id.has_value()) {
        arguments += request.subcommand_id.value() + " ";
      }

      if (request.message.has_value()) {
        arguments += request.message.value();
      }

      work.exec(
          "INSERT INTO actions(user_id, channel_id, command, arguments, "
          "full_message) VALUES (" +
          std::to_string(request.user.get_id()) + ", " +
          std::to_string(request.channel.get_id()) + ", '" +
          request.command_id + "', '" + arguments + "', '" +
          request.irc_message.message + "')");

      work.commit();

      return (*command)->run(bundle, request);
    }
  }
}
