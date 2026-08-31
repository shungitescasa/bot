#include <chrono>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "core/bot.hpp"
#include "core/builtin.hpp"
#include "core/command.hpp"
#include "core/config.hpp"
#include "core/data/database.hpp"
#include "core/data/event.hpp"
#include "core/event.hpp"
#include "core/externalapi/twitch.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"
#include "core/message.hpp"
#include "core/utils.hpp"
#include "scriptvm/client.hpp"

int main(int argc, char *argv[]) {
  bot::Logger log("Main");
  log.info("Starting up...");

  bot::Configuration &cfg = bot::Configuration::get_instance();
  cfg.load_from_args(argc, argv);

  if (cfg.irc.host.empty() || cfg.irc.port.empty() || cfg.irc.nick.empty() ||
      cfg.irc.pass.empty()) {
    throw std::runtime_error(
        "irc.host, irc.port, irc.nick, irc.pass must be set for IRC chatbot");
  }

  std::shared_ptr<bot::irc::IRCChatBot> chatbot =
      std::make_shared<bot::irc::IRCChatBot>(cfg.irc.host, cfg.irc.port,
                                             cfg.irc.nick, cfg.irc.pass);

  bot::externalapi::twitch::HelixClient &twitch_api =
      bot::externalapi::twitch::HelixClient::get_instance();
  twitch_api.set_token(cfg.twitch.token);

  bot::RPCChatBotServer rpc_server(chatbot, cfg.rpc.client_port);

  bot::CommandLoader command_loader;
  ADD_COMMAND(command_loader, PingCommand)

  scriptvm::RPCClient &script_vm = scriptvm::RPCClient::get_instance();
  script_vm.connect(cfg.rpc.host, cfg.rpc.port);

  chatbot->on_connect([&]() {
    log.info("Connected!");

    // wait for NICK announce
    std::this_thread::sleep_for(std::chrono::seconds(1));

    chatbot->join(chatbot->get_me());

    bot::data::DatabaseConnection conn = bot::data::create_connection();
    bot::data::DatabaseRows rows =
        conn->exec("SELECT name, alias_id FROM rooms WHERE parted_at IS NULL");

    int i = 0;

    for (bot::data::DatabaseRow row : rows) {
      if (i >= 5) {
        log.info("Initial JOIN cooldown... (30 seconds)");
        std::this_thread::sleep_for(std::chrono::seconds(30));
        i = 0;
      }

      std::string name = row.at("name");
      log.info(std::format("Joining #{}...", name));
      chatbot->join({name});

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      i++;
    }
  });

  chatbot->on_chat_message(
      [&](bot::Message<bot::MessageType::ChatMessage> message) {
        log.debug(std::format("{} <{}>: {}", message.source.login,
                              message.sender.login, message.contents));

        bot::data::DatabaseConnection conn = bot::data::create_connection();
        bot::Requester requester{message, conn};
        if (requester.room.parted_at.has_value()) return;

        if (!script_vm.is_alive()) {
          log.info("scriptVM RPC server is not alive! Reconnecting...");
          script_vm.connect();
        }

        // combining commands
        bot::CommandDataVec remote_commands = script_vm.list();

        bot::CommandDataVec all_commands;
        all_commands.reserve(remote_commands.size() +
                             command_loader.get_commands().size());

        for (const bot::CommandBox &c : command_loader.get_commands())
          all_commands.push_back(c->data());

        all_commands.insert(all_commands.end(), remote_commands.begin(),
                            remote_commands.end());

        // parsing request
        std::optional<bot::Request> request =
            bot::Request::create(all_commands, message, requester);

        bot::Response response;

        if (request.has_value()) {
          if (command_loader.has(request->command_id)) {
            response = command_loader.run(*request);
          } else {
            response = script_vm.execute(*request);
          }
        }

        if (response.is_single()) {
          chatbot->send_message("#" + message.source.login,
                                response.get_single());
        } else if (response.is_multiple()) {
          for (const std::string &text : response.get_multiple()) {
            chatbot->send_message("#" + message.source.login, text);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
          }
        }
      });

  bot::RSSEventRepository event_repository;
  event_repository.on_event([log, chatbot](
                                const std::string &type,
                                const std::string &name,
                                const std::vector<bot::RSSItem> &items) {
    std::vector<bot::data::Event> events = bot::data::get_events(type, name);
    constexpr int max_events_per_announce = 5;
    int events_exceed = items.size() - max_events_per_announce;

    for (bot::data::Event event : events) {
      int announcement_counter = 0;
      for (bot::RSSItem item : items) {
        if (announcement_counter >= max_events_per_announce) break;

        std::string event_msg = event.create_event_message(type, name, item);

        std::vector<std::string> user_lines =
            bot::utils::string::separate_by_length(event_msg, event.subs, "",
                                                   " ", 400);

        for (const std::string &line : user_lines) {
          chatbot->send_message(event.room_name, event_msg + line);
        }
      }

      if (events_exceed >= max_events_per_announce) {
        chatbot->send_message(
            event.room_name,
            std::format("...and {} more announcements", events_exceed));
      }
    }
  });

  std::vector<std::thread> threads;
  threads.push_back(std::thread(&bot::RPCChatBotServer::run, &rpc_server));
  threads.push_back(std::thread(&bot::ChatBot::connect, chatbot));
  threads.push_back(
      std::thread(&bot::RSSEventRepository::poll, event_repository));

  for (auto &t : threads) {
    if (t.joinable()) t.join();
  }

  return 0;
}
