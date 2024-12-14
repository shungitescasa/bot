#pragma once

#include <exception>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../commands/response_error.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"

namespace bot {
  namespace mod {
    class User : public command::Command {
        std::string get_name() const override { return "userid"; }

        int get_delay_seconds() const override { return 10; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!request.message.has_value()) {
            throw ResponseException<ResponseError::NOT_ENOUGH_ARGUMENTS>(
                request, bundle.localization, command::CommandArgument::VALUE);
          }

          std::vector<std::string> parts =
              utils::string::split_text(request.message.value(), ',');

          std::vector<std::string> ids;
          std::vector<std::string> logins;

          for (const std::string &part : parts) {
            if (ids.size() + logins.size() >= 3) break;
            try {
              int id = std::stoi(part);
              ids.push_back(part);
            } catch (std::exception e) {
              logins.push_back(part);
            }
          }

          std::string query;

          if (!ids.empty()) {
            query += "id=";
            query += utils::string::join_vector(ids, ',');

            if (!logins.empty()) query += "&";
          }

          if (!logins.empty()) {
            query += "login=";
            query += utils::string::join_vector(logins, ',');
          }

          if (query.empty()) {
            throw ResponseException<ResponseError::INCORRECT_ARGUMENT>(
                request, bundle.localization, request.message.value());
          }

          cpr::Response response =
              cpr::Get(cpr::Url{"https://api.ivr.fi/v2/twitch/user?" + query});

          if (response.status_code != 200) {
            throw ResponseException<ResponseError::EXTERNAL_API_ERROR>(
                request, bundle.localization, response.status_code,
                response.status_line);
          }

          nlohmann::json j = nlohmann::json::parse(response.text);

          std::vector<std::string> msgs;

          for (const auto &x : j) {
            std::string name = x["login"];
            std::string id = x["id"];

            std::string is_banned = x["banned"] ? "⛔" : "✅";
            std::string ban_reason;
            if (x.contains("banReason")) ban_reason = x["banReason"];

            std::string msg = is_banned + " " + name + " (" + id + ")" +
                              (ban_reason.empty() ? "" : ": ") + ban_reason;

            msgs.push_back(request.user.get_alias_name() + ": " + msg);
          }

          return command::Response(msgs);
        }
    };
  }
}
