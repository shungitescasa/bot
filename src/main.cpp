#include <iostream>
#include <optional>

#include "commands/command.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "localization/localization.hpp"

int main(int argc, char *argv[]) {
  std::cout << "hi world\n";

  std::optional<bot::Configuration> o_cfg =
      bot::parse_configuration_from_file(".env");

  if (!o_cfg.has_value()) {
    return -1;
  }

  bot::Configuration cfg = o_cfg.value();

  if (cfg.bot_password.empty() || cfg.bot_username.empty()) {
    std::cerr << "*** BOT_USERNAME and BOT_PASSWORD must be set!\n";
    return -1;
  }

  bot::irc::Client client(cfg.bot_username, cfg.bot_password);
  bot::command::CommandLoader command_loader;
  bot::loc::Localization localization("localization");

  client.join(cfg.bot_username);
  client.run();

  return 0;
}
