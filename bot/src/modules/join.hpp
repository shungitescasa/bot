#pragma once

#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../commands/command.hpp"
#include "../schemas/channel.hpp"

namespace bot {
  namespace mod {
    class Join : public command::Command {
        std::string get_name() const override { return "join"; }

        command::Response run(const InstanceBundle &bundle,
                              const command::Request &request) const override {
          if (!bundle.configuration.commands.join_allowed) {
            std::string owner = "";

            if (bundle.configuration.owner.name.has_value()) {
              owner = " " + bundle.localization
                                .get_formatted_line(
                                    request, loc::LineId::MsgOwner,
                                    {*bundle.configuration.owner.name})
                                .value();
            }

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::JoinNotAllowed,
                                        {owner})
                    .value());
          }

          if (!bundle.configuration.commands.join_allow_from_other_chats &&
              request.channel.get_alias_name() !=
                  bundle.irc_client.get_bot_username()) {
            return bundle.localization
                .get_formatted_line(request, loc::LineId::JoinFromOtherChat,
                                    {bundle.irc_client.get_bot_username()})
                .value();
          }

          pqxx::work work(request.conn);

          pqxx::result channels =
              work.exec("SELECT * FROM channels WHERE alias_id = " +
                        std::to_string(request.user.get_alias_id()));

          if (!channels.empty()) {
            schemas::Channel channel(channels[0]);

            if (channel.get_opted_out_at().has_value()) {
              work.exec("UPDATE channels SET opted_out_at = null WHERE id = " +
                        std::to_string(channel.get_id()));
              work.commit();

              bundle.irc_client.join(channel.get_alias_name());

              return command::Response(
                  bundle.localization
                      .get_formatted_line(request, loc::LineId::JoinRejoined,
                                          {})
                      .value());
            }

            return command::Response(
                bundle.localization
                    .get_formatted_line(request, loc::LineId::JoinAlreadyIn, {})
                    .value());
          }

          work.exec("INSERT INTO channels(alias_id, alias_name) VALUES (" +
                    std::to_string(request.user.get_alias_id()) + ", '" +
                    request.user.get_alias_name() + "')");
          work.commit();

          bundle.irc_client.join(request.user.get_alias_name());
          bundle.irc_client.say(
              request.user.get_alias_name(),
              bundle.localization
                  .get_formatted_line(request, loc::LineId::JoinResponseInChat,
                                      {})
                  .value());

          return command::Response(
              bundle.localization
                  .get_formatted_line(request, loc::LineId::JoinResponse, {})
                  .value());
        }
    };
  }
}
