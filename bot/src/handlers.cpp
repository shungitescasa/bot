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
#include "constants.hpp"
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

    std::string cid = parts[0];

    db::DatabaseRows cmds = conn->exec(
        "SELECT message FROM custom_commands WHERE name = $1 AND (channel_id "
        "= $2 OR is_global = TRUE)",
        {cid, std::to_string(requester.channel.get_id())});

    if (cmds.empty()) {
      return std::nullopt;
    }

    parts.erase(parts.begin());

    std::string initial_message = utils::string::join_vector(parts, ' ');

    db::DatabaseRow cmd = cmds[0];
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
      msg2.message = inside;

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

    if (!requester.has_value()) {
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

  void make_markov_response(
      const InstanceBundle &bundle,
      const irc::Message<irc::MessageType::Privmsg> &message,
      const schemas::Channel &channel,
      const schemas::ChannelPreferences &preference) {
    bool are_markov_responses_enabled =
        std::any_of(preference.get_features().begin(),
                    preference.get_features().end(), [](const int &x) {
                      return (schemas::ChannelFeature)x ==
                             schemas::ChannelFeature::MARKOV_RESPONSES;
                    });

    if (!are_markov_responses_enabled) return;

    bool are_random_markov_responses_enabled =
        std::any_of(preference.get_features().begin(),
                    preference.get_features().end(), [](const int &x) {
                      return (schemas::ChannelFeature)x ==
                             schemas::ChannelFeature::RANDOM_MARKOV_RESPONSES;
                    });

    std::string prefix = "@" + bundle.irc_client.get_bot_username() + ",";
    bool intended_call = message.message.substr(0, prefix.length()) == prefix;

    int random = -1;
    std::string question;

    if (are_random_markov_responses_enabled && !intended_call) {
      std::random_device dev;
      std::mt19937 rng(dev());

      std::uniform_int_distribution<std::mt19937::result_type> dist(0, 100);
      random = dist(rng);

      if (random > MARKOV_RESPONSE_CHANCE) return;
      question = message.message;
    } else {
      question =
          message.message.substr(prefix.length(), message.message.length());
    }

    if (random == -1 && !intended_call) return;

    cpr::Response response =
        cpr::Post(cpr::Url{"https://markov.ilotterytea.kz/api/v1/generate"},
                  cpr::Multipart{{"question", question}, {"max_length", 200}});

    if (response.status_code != 200) return;

    nlohmann::json j = nlohmann::json::parse(response.text);

    std::string answer;
    auto answer_field = j["data"]["answer"];

    if (answer_field.is_null())
      answer = "...";
    else
      answer = answer_field;

    bundle.irc_client.say(message.source.login,
                          message.sender.login + ": " + answer);
  }
}
