#pragma once

#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"
#include "../schemas/stream.hpp"

namespace bot {
  namespace mod {
    class Event : public command::Command {
        std::string get_name() const override { return "event"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        std::vector<std::string> get_subcommand_ids() const override {
          return {"on", "off"};
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
                request, bundle.localization, command::CommandArgument::TARGET);
          }

          const std::string &message = request.message.value();
          std::vector<std::string> s = utils::string::split_text(message, ' ');

          std::string target;
          schemas::EventType type;

          std::vector<std::string> target_and_type =
              utils::string::split_text(s[0], ':');

          if (target_and_type.size() != 2) {
            throw ResponseException<ResponseError::INCORRECT_ARGUMENT>(
                request, bundle.localization, s[0]);
          }

          s.erase(s.begin());

          target = target_and_type[0];
          type = schemas::string_to_event_type(target_and_type[1]);

          std::string t = target_and_type[0] + ":" + target_and_type[1];

          auto channels = bundle.helix_client.get_users({target});
          api::twitch::schemas::User channel;

          if (channels.empty() && type != schemas::EventType::CUSTOM) {
            throw ResponseException<ResponseError::NOT_FOUND>(
                request, bundle.localization, t);
          }

          pqxx::work work(request.conn);
          std::string query;

          if (type != schemas::CUSTOM) {
            channel = channels[0];

            query = "SELECT id FROM events WHERE channel_id = " +
                    std::to_string(request.channel.get_id()) +
                    " AND target_alias_id = " + std::to_string(channel.id) +
                    " AND event_type = " + std::to_string(type);
          } else {
            query = "SELECT id FROM events WHERE channel_id = " +
                    std::to_string(request.channel.get_id()) +
                    " AND custom_alias_id = '" + target +
                    "' AND event_type = " + std::to_string(type);
          }

          pqxx::result event = work.exec(query);

          if (subcommand_id == "on") {
            if (!event.empty()) {
              throw ResponseException<ResponseError::NAMESAKE_CREATION>(
                  request, bundle.localization, t);
            }

            if (s.empty()) {
              throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                  request, bundle.localization,
                  command::CommandArgument::MESSAGE);
            }

            std::string m = utils::string::str(s.begin(), s.end(), ' ');

            if (type != schemas::CUSTOM) {
              query =
                  "INSERT INTO events (channel_id, target_alias_id, "
                  "event_type, "
                  "message) VALUES (" +
                  std::to_string(request.channel.get_id()) + ", " +
                  std::to_string(channel.id) + ", " + std::to_string(type) +
                  ", '" + m + "')";
            } else {
              query =
                  "INSERT INTO events (channel_id, custom_alias_id, "
                  "event_type, "
                  "message) VALUES (" +
                  std::to_string(request.channel.get_id()) + ", '" + target +
                  "', " + std::to_string(type) + ", '" + m + "')";
            }

            work.exec(query);
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::EventOn, {t})
                    .value());
          } else if (subcommand_id == "off") {
            if (event.empty()) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, t);
            }

            work.exec("DELETE FROM events WHERE id = " +
                      std::to_string(event[0][0].as<int>()));
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::EventOff, {t})
                    .value());
          }

          throw ResponseException<ResponseError::SOMETHING_WENT_WRONG>(
              request, bundle.localization);
        }
    };
  }
}
