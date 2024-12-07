#pragma once

#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"

namespace bot {
  namespace mod {
    class Massping : public command::Command {
        std::string get_name() const override { return "massping"; }

        schemas::PermissionLevel get_permission_level() const override {
          return schemas::PermissionLevel::MODERATOR;
        }

        int get_delay_seconds() const override { return 1; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          auto chatters = bundle.helix_client.get_chatters(
              request.channel.get_alias_id(), bundle.irc_client.get_bot_id());

          std::string m;

          if (request.message.has_value()) {
            m = request.message.value() + " ·";
          }

          std::string base = "📣 " + m + " ";
          std::vector<std::string> msgs = {""};
          int index = 0;

          for (const auto &chatter : chatters) {
            const std::string &current_msg = msgs.at(index);
            std::string x = "@" + chatter.login;

            if (base.length() + current_msg.length() + 1 + x.length() >= 500) {
              index += 1;
            }

            if (index > msgs.size() - 1) {
              msgs.push_back(x);
            } else {
              msgs[index] = current_msg + " " + x;
            }
          }

          std::vector<std::string> msgs2;

          for (const auto &m : msgs) {
            msgs2.push_back(base + m);
          }

          return command::Response(msgs2);
        }
    };
  }
}
