#pragma once

#include <cpr/cpr.h>

#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/command.hpp"
#include "core/utils.hpp"
#include "scriptvm/client.hpp"

namespace scriptvm::builtin {
  class ScriptExecutionCommand : public bot::Command {
    public:
      ScriptExecutionCommand() : Command("lua", 5, {"execute"}) {}
      const bot::Response run(const bot::Request &request) const override {
        bot::Configuration &cfg = bot::Configuration::get_instance();
        if (!cfg.script.allow_arbitrary_scripts &&
            request.requester.sender_right.level <
                static_cast<int>(bot::data::PermissionLevel::Trusted)) {
          return {std::runtime_error(
              "You have not enough rights to execute this command.")};
        }

        if (!request.contents.has_value()) {
          return {std::runtime_error("No script provided.")};
        }

        scriptvm::RPCClient &script_vm = scriptvm::RPCClient::get_instance();
        return script_vm.execute_untrusted_script(request.contents.value(),
                                                  request);
      };
  };

  class ScriptRemoteCommand : public bot::Command {
    public:
      ScriptRemoteCommand() : Command("luaimport", 0, {"import"}) {}
      const bot::Response run(const bot::Request &request) const override {
        if (!request.contents.has_value()) {
          return {std::runtime_error("No URL provided.")};
        }

        std::vector<std::string> parts = bot::utils::string::split_and_collect(
            request.contents.value(), ' ');

        std::string url = parts.front();
        parts.erase(parts.begin());
        std::string c = bot::utils::string::join(parts, " ");

        bot::Request r = request;
        r.contents = c.empty() ? std::nullopt : std::optional(c);

        bot::Configuration &cfg = bot::Configuration::get_instance();

        bool trusted_script = std::any_of(
            cfg.script.url_whitelist.begin(), cfg.script.url_whitelist.end(),
            [&url](const std::string &u) { return u == url; });

        if (!cfg.script.allow_arbitrary_scripts &&
            request.requester.sender_right.level <
                static_cast<int>(bot::data::PermissionLevel::Trusted) &&
            !trusted_script) {
          return {std::runtime_error(
              "You have not enough rights to execute this command.")};
        }

        // retrieving the script
        std::vector<std::string> mime_types = {"text/plain", "text/x-lua",
                                               "text/plain; charset=utf-8",
                                               "text/x-lua; charset=utf-8"};

        cpr::Response response = cpr::Get(
            cpr::Url{url},
            cpr::Header{{"Accept", bot::utils::string::join(mime_types, ",")},
                        {"User-Agent", cfg.instance.user_agent}});

        if (response.status_code > 399) {
          return {std::runtime_error(std::format(
              "Failed to retrieve script ({})", response.status_code))};
        }

        std::string content_type = response.header.at("Content-Type");
        if (!std::any_of(mime_types.begin(), mime_types.end(),
                         [&content_type](const std::string &c) {
                           return c == content_type;
                         })) {
          return {std::runtime_error(
              std::format("Content-Type is not allowed: {}", content_type))};
        }

        r.meta.insert({"lua-id", url});
        r.meta.insert({"trusted-script", trusted_script ? "true" : "false"});

        scriptvm::RPCClient &script_vm = scriptvm::RPCClient::get_instance();
        return script_vm.execute_untrusted_script(response.text, r);
      };
  };
}
