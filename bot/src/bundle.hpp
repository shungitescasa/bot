#pragma once

#include "chat.hpp"
namespace bot {
  namespace command {
    class CommandLoader;
  }

  namespace loc {
    class Localization;
  }

  class InstanceBundle;
}

#include "api/kick.hpp"
#include "api/twitch/helix_client.hpp"
#include "commands/command.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "localization/localization.hpp"

namespace bot {
  struct InstanceBundle {
      chat::ChatClient &irc_client;
      const api::twitch::HelixClient &helix_client;
      const api::KickAPIClient &kick_api_client;
      const bot::loc::Localization &localization;
      const Configuration &configuration;
      const command::CommandLoader &command_loader;
  };
}
