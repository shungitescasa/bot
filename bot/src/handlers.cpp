#include "handlers.hpp"

#include <algorithm>
#include <exception>
#include <optional>
#include <pqxx/pqxx>
#include <random>
#include <string>
#include <vector>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "commands/request.hpp"
#include "commands/request_util.hpp"
#include "cpr/api.h"
#include "cpr/multipart.h"
#include "cpr/response.h"
#include "irc/message.hpp"
#include "localization/line_id.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "schemas/channel.hpp"
#include "utils/string.hpp"

namespace bot::handlers {
  void handle_private_message(
      const InstanceBundle &bundle,
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &message,
      pqxx::connection &conn) {
    if (utils::string::string_contains_sql_injection(message.message)) {
      log::warn("PrivateMessageHandler",
                "Received the message in #" + message.source.login +
                    " with SQL injection: " + message.message);
      return;
    }

    std::optional<command::Request> request =
        command::generate_request(command_loader, message, conn);

    if (request.has_value()) {
      try {
        auto response = command_loader.run(bundle, request.value());

        if (response.has_value()) {
          if (response->is_single()) {
            bundle.irc_client.say(message.source.login, response->get_single());
          } else if (response->is_multiple()) {
            for (const std::string &msg : response->get_multiple()) {
              bundle.irc_client.say(message.source.login, msg);
            }
          }
        }
      } catch (const std::exception &e) {
        std::string line =
            bundle.localization
                .get_formatted_line(request.value(), loc::LineId::ErrorTemplate,
                                    {e.what()})
                .value();

        bundle.irc_client.say(message.source.login, line);
      }
    }

    pqxx::work work(conn);
    pqxx::result channels =
        work.exec("SELECT * FROM channels WHERE alias_id = " +
                  std::to_string(message.source.id));

    if (!channels.empty()) {
      schemas::Channel channel(channels[0]);

      pqxx::result channel_preferences = work.exec(
          "SELECT * FROM channel_preferences WHERE "
          "channel_id = " +
          std::to_string(channel.get_id()));

      schemas::ChannelPreferences preference(channel_preferences[0]);

      pqxx::result cmds =
          work.exec("SELECT message FROM custom_commands WHERE name = '" +
                    message.message + "' AND channel_id = '" +
                    std::to_string(channel.get_id()) + "'");

      if (!cmds.empty()) {
        std::string msg = cmds[0][0].as<std::string>();

        bundle.irc_client.say(message.source.login, msg);
      } else {
        make_markov_response(bundle, message, channel, preference);
      }
    }

    work.commit();
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

      std::uniform_int_distribution<std::mt19937::result_type> dist(0, 10);
      random = dist(rng);

      if (random != 0) return;
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
