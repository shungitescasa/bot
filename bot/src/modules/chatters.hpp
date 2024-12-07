#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"
#include "cpr/api.h"
#include "cpr/multipart.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"

namespace bot::mod {
  class Chatters : public command::Command {
      std::string get_name() const override { return "chatters"; }

      schemas::PermissionLevel get_permission_level() const override {
        return schemas::PermissionLevel::USER;
      }

      int get_delay_seconds() const override { return 10; }

      command::Response run(const InstanceBundle &bundle,
                            const command::Request &request) const override {
        if (!bundle.configuration.url.paste_service.has_value()) {
          throw ResponseException<ResponseError::ILLEGAL_COMMAND>(
              request, bundle.localization);
        }

        auto chatters = bundle.helix_client.get_chatters(
            request.channel.get_alias_id(), bundle.irc_client.get_bot_id());

        std::string body;

        for (const auto &chatter : chatters) {
          body += chatter.login + '\n';
        }

        std::time_t t = std::time(nullptr);
        std::tm *now = std::localtime(&t);

        std::ostringstream oss;

        oss << std::put_time(now, "%d.%m.%Y %H:%M:%S");

        cpr::Multipart multipart = {
            {"paste", body},
            {"title", request.channel.get_alias_name() + "'s chatter list on " +
                          oss.str()}};

        cpr::Response response = cpr::Post(
            cpr::Url{*bundle.configuration.url.paste_service + "/paste"},
            multipart);

        if (response.status_code == 201) {
          nlohmann::json j = nlohmann::json::parse(response.text);

          std::string id = j["data"]["id"];

          std::string url = *bundle.configuration.url.paste_service + "/" + id;

          return command::Response(
              bundle.localization
                  .get_formatted_line(request, loc::LineId::ChattersResponse,
                                      {url})
                  .value());
        } else {
          throw ResponseException<ResponseError::EXTERNAL_API_ERROR>(
              request, bundle.localization, response.status_code,
              response.status_line);
        }
      }
  };
}
