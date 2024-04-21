#include <optional>

#include "../irc/message.hpp"
#include "command.hpp"
#include "request.hpp"

namespace bot::command {
  std::optional<Request> generate_request(
      const command::CommandLoader &command_loader,
      const irc::Message<irc::MessageType::Privmsg> &irc_message);
}
