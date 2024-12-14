#pragma once

#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class Settings : public command::Command {
        std::string get_name() const override { return "set"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        std::vector<std::string> get_subcommand_ids() const override {
          return {"locale", "prefix", "feature"};
        }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::CommandArgument::VALUE);
          }

          pqxx::work work(request.conn);

          std::vector<std::string> parts =
              utils::string::split_text(request.message.value(), ' ');

          std::string &value = parts[0];

          if (request.subcommand_id == "locale") {
            auto locals = bundle.localization.get_loaded_localizations();
            if (!std::any_of(locals.begin(), locals.end(),
                             [&value](const auto &x) { return x == value; })) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, value);
            }

            work.exec_params(
                "UPDATE channel_preferences SET locale = $1 WHERE channel_id = "
                "$2",
                value, request.channel.get_id());

            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::SetLocale,
                                        {value})
                    .value());
          } else if (request.subcommand_id == "prefix") {
            work.exec_params(
                "UPDATE channel_preferences SET prefix = $1 WHERE channel_id = "
                "$2",
                value, request.channel.get_id());

            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::SetPrefix,
                                        {value})
                    .value());
          } else if (request.subcommand_id == "feature") {
            std::optional<schemas::ChannelFeature> feature =
                schemas::string_to_channel_feature(value);
            if (!feature.has_value()) {
              throw ResponseException<ResponseError::NOT_FOUND>(
                  request, bundle.localization, value);
            }

            loc::LineId line_id;
            std::string query;

            if (std::any_of(request.channel_preferences.get_features().begin(),
                            request.channel_preferences.get_features().end(),
                            [&feature](const auto &x) {
                              return x == feature.value();
                            })) {
              line_id = loc::LineId::SetFeatureDisabled;
              query =
                  "UPDATE channel_preferences SET features = "
                  "array_remove(features, $1) WHERE channel_id = $2";
            } else {
              line_id = loc::LineId::SetFeatureEnabled;
              query =
                  "UPDATE channel_preferences SET features = "
                  "array_append(features, $1) WHERE channel_id = $2";
            }

            work.exec_params(query, (int)feature.value(),
                             request.channel.get_id());
            work.commit();

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, line_id, {value})
                    .value());
          }

          work.commit();
          throw ResponseException<ResponseError::SOMETHING_WENT_WRONG>(
              request, bundle.localization);
        }
    };
  }
}
