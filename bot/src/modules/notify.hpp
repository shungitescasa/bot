#pragma once

#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"
#include "../schemas/stream.hpp"

namespace bot {
  namespace mod {
    class Notify : public command::Command {
        std::string get_name() const override { return "notify"; }

        std::vector<std::string> get_subcommand_ids() const override {
          return {"sub", "unsub"};
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

          if (channels.empty() && type < schemas::EventType::GITHUB) {
            throw ResponseException<ResponseError::NOT_FOUND>(
                request, bundle.localization, t);
          }

          pqxx::work work(request.conn);
          std::string query;

          if (type < schemas::GITHUB) {
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

          pqxx::result events = work.exec(query);

          if (events.empty()) {
            throw ResponseException<ResponseError::NOT_FOUND>(
                request, bundle.localization, t);
          }

          pqxx::row event = events[0];

          pqxx::result subs =
              work.exec("SELECT id FROM event_subscriptions WHERE event_id = " +
                        std::to_string(event[0].as<int>()) + " AND user_id = " +
                        std::to_string(request.user.get_id()));

          if (subcommand_id == "sub") {
            if (!subs.empty()) {
              throw ResponseException<ResponseError::NAMESAKE_CREATION>(
                  request, bundle.localization, t);
            }

            work.exec(
                "INSERT INTO event_subscriptions(event_id, user_id) VALUES (" +
                std::to_string(event[0].as<int>()) + ", " +
                std::to_string(request.user.get_id()) + ")");
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::NotifySub, {t})
                    .value());
          } else if (subcommand_id == "unsub") {
            if (subs.empty()) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, t);
            }

            work.exec("DELETE FROM event_subscriptions WHERE id = " +
                      std::to_string(subs[0][0].as<int>()));
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::NotifyUnsub, {t})
                    .value());
          }

          throw ResponseException<ResponseError::SOMETHING_WENT_WRONG>(
              request, bundle.localization);
        }
    };
  }
}
