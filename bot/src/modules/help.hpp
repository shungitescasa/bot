#pragma once

#include <string>
#include <variant>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class Help : public command::Command {
        std::string get_name() const override { return "help"; }

        std::variant<std::vector<std::string>, std::string> run(
            const InstanceBundle &bundle,
            const command::Request &request) const override {
          if (!bundle.configuration.url.help.has_value()) {
            throw ResponseException<ResponseError::ILLEGAL_COMMAND>(
                request, bundle.localization);
          }

          return bundle.localization
              .get_formatted_line(request, loc::LineId::HelpResponse,
                                  {*bundle.configuration.url.help})
              .value();
        }
    };
  }
}
