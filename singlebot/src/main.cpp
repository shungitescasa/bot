#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/builtin.hpp"
#include "core/command.hpp"
#include "core/config.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"
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

  scriptvm::RPCClient script_vm(cfg.rpc.host, cfg.rpc.port);

  chatbot.on_chat_message(
      [&](bot::Message<bot::MessageType::ChatMessage> message) {
        std::println("#{} <{}>: {}", message.source.login, message.sender.login,
                     message.contents);

        bot::CommandVec commands;
        commands.insert(commands.end(), command_loader.get_commands().begin(),
                        command_loader.get_commands().end());

        bot::Requester requester{message};
        std::optional<bot::Request> request =
            bot::Request::create(commands, message, requester);

        if (request.has_value()) {
          auto response = command_loader.run(*request);

          if (response.is_single()) {
            chatbot.send_message("#" + message.source.login,
                                 response.get_single());
          }
        }

        auto response = script_vm.execute_untrusted_script(message.contents);
        if (response.has_value()) {
          std::println("script vm: {}", response.value());
        }
      });

  chatbot.connect();
}
