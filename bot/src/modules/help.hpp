#pragma once

#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"

namespace bot {
  namespace mod {
    class Help : public command::Command {
        std::string get_name() const override { return "help"; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!bundle.configuration.url.help.has_value()) {
            throw ResponseException<ResponseError::ILLEGAL_COMMAND>(
                request, bundle.localization);
          }

          return command::Response(
              bundle.localization
                  .get_formatted_line(request, loc::LineId::HelpResponse,
                                      {*bundle.configuration.url.help})
                  .value());
        }
    };
  }
}
