#pragma once

#include <string>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"

namespace bot {
  namespace mod {
    class MinecraftServerCheck : public command::Command {
        std::string get_name() const override { return "mcsrv"; }

        int get_delay_seconds() const override { return 10; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::CommandArgument::VALUE);
          }

          cpr::Response response = cpr::Get(cpr::Url{
              "https://api.mcsrvstat.us/3/" + request.message.value()});

          if (response.status_code != 200) {
            throw ResponseException<ResponseError::EXTERNAL_API_ERROR>(
                request, bundle.localization, response.status_code,
                response.status_line);
          }

          nlohmann::json j = nlohmann::json::parse(response.text);

          std::string online = j["online"] ? "✅" : "⛔";
          std::string ip = "IP N/A";

          if (j.contains("ip")) ip = j["ip"];

          std::string player_count = "PLAYERS N/A";

          if (j.contains("players")) {
            auto players = j["players"];
            player_count = std::to_string(players["online"].get<int>());
            player_count += "/";
            player_count += std::to_string(players["max"].get<int>());
          }

          std::string version = "VERSION N/A";

          if (j.contains("protocol")) {
            auto protocol = j["protocol"];
            if (protocol.contains("name")) version = protocol["name"];
          }

          std::string motd = "MOTD N/A";

          if (j.contains("motd")) {
            auto motd_json = j["motd"];
            if (motd_json.contains("clean")) {
              motd.clear();
              for (const auto &line : motd_json["clean"]) {
                motd += line;
                motd += " / ";
              }
              motd = motd.substr(0, motd.size() - 3);
              motd = "\"" + motd + "\"";
            }
          }

          std::string msg = online + " " + request.message.value() + " (" + ip +
                            ") | " + player_count + " | " + motd + " | " +
                            version;

          return command::Response(msg);
        }
    };
  }
}
