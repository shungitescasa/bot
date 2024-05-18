#pragma once

#include <string>
#include <variant>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class CustomCommand : public command::Command {
        std::string get_name() const override { return "scmd"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        std::vector<std::string> get_subcommand_ids() const override {
          return {"new", "remove"};
        }

        std::variant<std::vector<std::string>, std::string> run(
            const InstanceBundle &bundle,
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
          pqxx::result cmds = work.exec(
              "SELECT id FROM custom_commands WHERE name = '" + name +
              "' AND channel_id = " + std::to_string(request.channel.get_id()));

          if (subcommand_id == "new") {
            if (!cmds.empty()) {
              throw ResponseException<ResponseError::NAMESAKE_CREATION>(
                  request, bundle.localization, name);
            }

            if (s.empty()) {
              throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                  request, bundle.localization,
                  command::CommandArgument::MESSAGE);
            }

            std::string m = utils::string::str(s.begin(), s.end(), ' ');

            work.exec(
                "INSERT INTO custom_commands(channel_id, name, message) VALUES "
                "(" +
                std::to_string(request.channel.get_id()) + ", '" + name +
                "', '" + m + "')");
            work.commit();

            return bundle.localization
                .get_formatted_line(request, loc::LineId::CustomcommandNew,
                                    {name})
                .value();
          } else if (subcommand_id == "remove") {
            if (cmds.empty()) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, name);
            }

            work.exec("DELETE FROM custom_commands WHERE id = " +
                      std::to_string(cmds[0][0].as<int>()));
            work.commit();

            return bundle.localization
                .get_formatted_line(request, loc::LineId::CustomcommandDelete,
                                    {name})
                .value();
          }

          throw ResponseException<ResponseError::SOMETHING_WENT_WRONG>(
              request, bundle.localization);
        }
    };
  }
}
