#include <print>
#include <stdexcept>
#include <string>

#include "core/config.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"

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

  chatbot.on_chat_message(
      [&](bot::Message<bot::MessageType::ChatMessage> message) {
        std::println("#{} <{}>: {}", message.source.login, message.sender.login,
                     message.contents);

        if (message.contents == "ping") {
          chatbot.send_message("#" + message.source.login, "pong");
        }
      });

  chatbot.connect();
}
