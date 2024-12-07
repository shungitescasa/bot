#pragma once

#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class Timer : public command::Command {
        std::string get_name() const override { return "timer"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        std::vector<std::string> get_subcommand_ids() const override {
          return {"new", "remove"};
        }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.subcommand_id.has_value()) {
            throw ResponseException<NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::SUBCOMMAND);
          }

          const std::string &subcommand_id = request.subcommand_id.value();

          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::CommandArgument::NAME);
          }

          const std::string &message = request.message.value();
          std::vector<std::string> s = utils::string::split_text(message, ' ');

          std::string name = s[0];
          s.erase(s.begin());

          pqxx::work work(request.conn);
          pqxx::result timers = work.exec(
              "SELECT id FROM timers WHERE name = '" + name +
              "' AND channel_id = " + std::to_string(request.channel.get_id()));

          if (subcommand_id == "new") {
            if (!timers.empty()) {
              throw ResponseException<ResponseError::NAMESAKE_CREATION>(
                  request, bundle.localization, name);
            }

            if (s.empty()) {
              throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                  request, bundle.localization,
                  command::CommandArgument::INTERVAL);
            }

            int interval_s;

            try {
              interval_s = std::stoi(s[0]);
            } catch (std::exception e) {
              throw ResponseException<ResponseError::INCORRECT_ARGUMENT>(
                  request, bundle.localization, s[0]);
            }

            s.erase(s.begin());

            if (s.empty()) {
              throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                  request, bundle.localization,
                  command::CommandArgument::MESSAGE);
            }

            std::string m = utils::string::str(s.begin(), s.end(), ' ');

            work.exec(
                "INSERT INTO timers(channel_id, name, message, interval_sec) "
                "VALUES "
                "(" +
                std::to_string(request.channel.get_id()) + ", '" + name +
                "', '" + m + "', " + std::to_string(interval_s) + ")");
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::TimerNew, {name})
                    .value());
          } else if (subcommand_id == "remove") {
            if (timers.empty()) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, name);
            }

            work.exec("DELETE FROM timers WHERE id = " +
                      std::to_string(timers[0][0].as<int>()));
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::TimerDelete,
                                        {name})
                    .value());
          }

          throw ResponseException<ResponseError::SOMETHING_WENT_WRONG>(
              request, bundle.localization);
        }
    };
  }
}
