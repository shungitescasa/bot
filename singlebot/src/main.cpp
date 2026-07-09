#include <chrono>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "core/builtin.hpp"
#include "core/command.hpp"
#include "core/config.hpp"
#include "core/data/database.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"
#include "core/message.hpp"
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

  bot::irc::IRCChatBot chatbot(cfg.irc.host, cfg.irc.port, cfg.irc.nick,
                               cfg.irc.pass);

  bot::CommandLoader command_loader;
  ADD_COMMAND(command_loader, PingCommand)

  scriptvm::RPCClient &script_vm = scriptvm::RPCClient::get_instance();
  script_vm.connect(cfg.rpc.host, cfg.rpc.port);

  chatbot.on_connect([&]() {
    log.info("Connected!");

    chatbot.join(chatbot.get_me());
  });

  chatbot.on_chat_message(
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
          chatbot.send_message("#" + message.source.login,
                               response.get_single());
        } else if (response.is_multiple()) {
          for (const std::string &text : response.get_multiple()) {
            chatbot.send_message("#" + message.source.login, text);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
          }
        }
      });

  chatbot.connect();
}
