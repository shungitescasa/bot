#pragma once

#include <format>
#include <string>

#include "core/command.hpp"
#include "core/utils.hpp"
#include "scriptvm/client.hpp"

const auto START_TIME = std::chrono::steady_clock::now();

namespace bot::builtin {
  class PingCommand : public Command {
    public:
      PingCommand() : Command("ping") {}

      const Response run(const Request &request) const override {
        std::string response = "🏓 Pong! Uptime: ";

        // calculating uptime
        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - START_TIME)
                .count();
        response +=
            utils::chrono::humanize_timestamp(static_cast<long long>(elapsed));

        // scriptVM health check
        response += " · scriptVM: ";
        auto &scriptvm = scriptvm::RPCClient::get_instance();
        long long scriptvm_uptime = scriptvm.uptime();
        if (scriptvm_uptime == -1) {
          response += "N/A";
        } else {
          response +=
              std::format("{} ({} commands)",
                          utils::chrono::humanize_timestamp(scriptvm_uptime),
                          scriptvm.list().size());
        }

        return response;
      }
  };
}