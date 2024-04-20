#include <iostream>
#include <optional>

#include "config.hpp"
#include "irc/client.hpp"

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

  client.run();

  return 0;
}
