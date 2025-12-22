#include "handlers.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/request.hpp"
#include "commands/response.hpp"
#include "commands/response_error.hpp"
#include "cpr/api.h"
#include "cpr/multipart.h"
#include "cpr/response.h"
#include "database.hpp"
#include "irc/message.hpp"
#include "localization/line_id.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "schemas/channel.hpp"
#include "utils/string.hpp"

namespace bot::handlers {
  std::optional<command::Response> run_command(
      const InstanceBundle &bundle, command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      const command::Requester &requester,
      std::unique_ptr<db::BaseDatabase> &conn) {
    std::optional<command::Request> request =
        command::generate_request(command_loader, message, requester, conn);

    if (request.has_value()) {
      try {
        auto response = command_loader.run(bundle, request.value());

        return response;
      } catch (const std::exception &e) {
        bundle.irc_client.say(message.source.login, e.what());
        log::error("PrivMsg/" + request->command_id, e.what());
      }
    }

    return std::nullopt;
  }

  std::optional<std::string> handle_custom_commands(
      const InstanceBundle &bundle, command::CommandLoader &command_loader,
      std::unique_ptr<db::BaseDatabase> &conn,
      const command::Requester &requester,
      const irc::Message<irc::MessageType::Privmsg> &message) {
    std::vector<std::string> parts =
        utils::string::split_text(message.message, ' ');

    if (parts.empty()) {
      return std::nullopt;
    }

    std::string prefix = requester.channel_preferences.get_prefix();
    std::string cid = parts[0];
    std::string cid_without_prefix = cid;

    if (cid.size() > prefix.size() && cid.substr(0, prefix.size()) == prefix) {
      cid_without_prefix = "{prefix}" + cid.substr(prefix.size(), cid.size());
    }

    db::DatabaseRows cmds = conn->exec(
        "SELECT cc.name, cc.message, cca.name AS alias_name FROM "
        "custom_commands cc "
        "LEFT JOIN custom_command_aliases cca ON cca.command_id = cc.id "
        "WHERE (BINARY cc.name = $1 OR BINARY cc.name = $2 "
        "OR BINARY cca.name = $3 OR BINARY cca.name = $4) "
        "AND (cc.channel_id "
        "= $5 OR cc.is_global = TRUE)",
        {cid, cid_without_prefix, cid, cid_without_prefix,
         std::to_string(requester.channel.get_id())});

    if (cmds.empty()) {
      return std::nullopt;
    }

    db::DatabaseRow cmd = cmds[0];
    std::string cmd_name =
        cmd.at("alias_name").empty() ? cmd.at("name") : cmd.at("alias_name");
    if (cmd_name.length() > 8 && cmd_name.substr(0, 8) == "{prefix}" &&
        cmd_name.substr(0, requester.channel_preferences.get_prefix().size()) !=
            requester.channel_preferences.get_prefix()) {
      return std::nullopt;
    }

    parts.erase(parts.begin());

    std::string initial_message = utils::string::join_vector(parts, ' ');

    std::string msg = cmd.at("message");

    // parsing values
    std::regex pattern(R"(\{([^}]*)\})");
    std::string output;
    std::sregex_iterator iter(msg.begin(), msg.end(), pattern);
    std::sregex_iterator end;

    int last_pos = 0;
    for (; iter != end; ++iter) {
      auto m = *iter;
      output += msg.substr(last_pos, m.position() - last_pos);

      std::string inside = m[1].str();

      int placeholder_pos = inside.find("$1");
      if (placeholder_pos != std::string::npos) {
        inside.replace(placeholder_pos, 3, initial_message);
      }

      irc::Message<irc::MessageType::Privmsg> msg2 = message;
      msg2.message = requester.channel_preferences.get_prefix() + inside;

      std::optional<command::Response> response =
          run_command(bundle, command_loader, msg2, requester, conn);

      std::string answer = inside;

      if (response.has_value()) {
        if (response->is_single()) {
          answer = response->get_single();
        } else if (response->is_multiple()) {
          answer = "[multiple strings]";
        }
      }

      output += answer;

      last_pos = m.position() + m.length();
    }

    output += msg.substr(last_pos);

    return output;
  }

  void handle_private_message(
      const InstanceBundle &bundle, command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message) {
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(bundle.configuration);

    std::optional<command::Requester> requester =
        command::get_requester(message, conn, bundle.configuration);

    if (!requester.has_value() ||
        requester->user.get_alias_name() == bundle.irc_client.get_username()) {
      return;
    }

    std::optional<command::Response> response =
        run_command(bundle, command_loader, message, *requester, conn);

    if (response.has_value()) {
      if (response->is_single()) {
        bundle.irc_client.say(message.source.login, response->get_single());
      } else if (response->is_multiple()) {
        for (const std::string &msg : response->get_multiple()) {
          bundle.irc_client.say(message.source.login, msg);
        }
      }

      return;
    }

    std::optional<std::string> custom_command_response = handle_custom_commands(
        bundle, command_loader, conn, *requester, message);

    if (custom_command_response.has_value()) {
      bundle.irc_client.say(message.source.login, *custom_command_response);
      return;
    }

    conn->close();
  }
}
