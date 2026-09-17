#pragma once

#include <format>
#include <memory>
#include <string>

#include "core/command.hpp"
#include "core/config.hpp"
#include "core/irc/bot.hpp"
#include "core/utils.hpp"
#include "cpr/api.h"
#include "cpr/response.h"
#include "scriptvm/client.hpp"

const auto START_TIME = std::chrono::steady_clock::now();

namespace bot::builtin {
  class PingCommand : public Command {
    public:
      PingCommand(std::shared_ptr<irc::IRCChatBot> chatbot)
          : Command("ping"), chatbot(chatbot) {}

      const Response run(const Request &request) const override {
        auto &cfg = Configuration::get_instance();

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

        // eventpoller health check
        if (cfg.rss.url.has_value()) {
          response += " · Events: ";

          try {
            cpr::Response http_response =
                cpr::Get(cpr::Url{*cfg.rss.url + "/health"});

            if (http_response.status_code > 399) {
              response += std::format("ERR ({})", http_response.status_code);
            } else {
              auto success = std::chrono::steady_clock::now();
              elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                            success - now)
                            .count();
              response += std::format("{}ms", static_cast<long long>(elapsed));
            }
          } catch (std::exception &e) {
            response += "N/A";
          }
        }

        // IRC latency
        if (chatbot->get_latency() > 0) {
          response += std::format(" · Latency: {}ms", chatbot->get_latency());
        }

        // room count
        int room_count = chatbot->room_count();
        if (room_count == 1) {
          response += std::format(" · {} channel", room_count);
        } else if (room_count > 1) {
          response += std::format(" · {} channels", room_count);
        }

        return response;
      }

    private:
      std::shared_ptr<irc::IRCChatBot> chatbot;
  };
}
