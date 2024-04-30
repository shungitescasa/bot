#pragma once

#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <variant>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../utils/chrono.hpp"

namespace bot {
  namespace mod {
    class Ping : public command::Command {
        std::string get_name() const override { return "ping"; }

        std::variant<std::vector<std::string>, std::string> run(
            const InstanceBundle &bundle,
            const command::Request &request) const override {
          auto now = std::chrono::steady_clock::now();
          auto duration = now - START_TIME;
          auto seconds =
              std::chrono::duration_cast<std::chrono::seconds>(duration);
          std::string uptime = utils::chrono::format_timestamp(seconds.count());

          struct rusage usage;
          getrusage(RUSAGE_SELF, &usage);

          int used_memory = usage.ru_maxrss / 1024;

          return bundle.localization
              .get_formatted_line(request, loc::LineId::PingResponse,
                                  {uptime, std::to_string(used_memory)})
              .value();
        }
    };
  }
}
