#include <stdexcept>
#include <string>

#include "core/config.hpp"
#include "core/irc/bot.hpp"
#include "core/log.hpp"

int main(int argc, char *argv[]) {
  bot::Logger log("Main");
  log.info("Starting up...");

  std::string config_path = ".env";

  bot::Configuration &cfg = bot::Configuration::get_instance();
  cfg.load_file(config_path);

  if (cfg.irc.host.empty() || cfg.irc.port.empty() || cfg.irc.nick.empty() ||
      cfg.irc.pass.empty()) {
    throw std::runtime_error(
        "irc.host, irc.port, irc.nick, irc.pass must be set for IRC chatbot");
  }

  bot::irc::IRCChatBot chatbot(cfg.irc.host, cfg.irc.port, cfg.irc.nick,
                               cfg.irc.pass);

  chatbot.connect();
}
