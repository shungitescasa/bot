#include <iostream>
#include <optional>

#include "bundle.hpp"
#include "commands/command.hpp"
#include "config.hpp"
#include "handlers.hpp"
#include "irc/client.hpp"
#include "irc/message.hpp"
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

  if (cfg.database.name.empty() || cfg.database.user.empty() ||
      cfg.database.password.empty() || cfg.database.host.empty() ||
      cfg.database.port.empty()) {
    std::cerr
        << "*** DB_NAME, DB_USER, DB_PASSWORD, DB_HOST, DB_PORT must be set!\n";
    return -1;
  }

  bot::irc::Client client(cfg.bot_username, cfg.bot_password);
  bot::command::CommandLoader command_loader;
  bot::loc::Localization localization("localization");

  client.join(cfg.bot_username);

  client.on<bot::irc::MessageType::Privmsg>(
      [&client, &command_loader, &localization](
          const bot::irc::Message<bot::irc::MessageType::Privmsg> &message) {
        bot::InstanceBundle bundle{client, localization};
        bot::handlers::handle_private_message(bundle, command_loader, message);
      });

  client.run();

  return 0;
}
