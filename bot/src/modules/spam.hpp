#pragma once

#include <exception>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class Spam : public command::Command {
        std::string get_name() const override { return "spam"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        int get_delay_seconds() const override { return 10; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::MESSAGE);
          }

          std::vector<std::string> parts =
              utils::string::split_text(request.message.value(), ' ');

          int count = SPAM_DEFAULT_COUNT;
          std::string message = request.message.value();

          try {
            count = std::stoi(parts[0]);

            if (count > SPAM_MAX_COUNT) count = SPAM_MAX_COUNT;
            message = utils::string::join_vector(
                {parts.begin() + 1, parts.end()}, ' ');
          } catch (std::exception &e) {
          }

          std::vector<std::string> output;

          for (int i = 0; i < count; i++) {
            output.push_back(message);
          }

          return command::Response(output);
        }
    };
  }
}
