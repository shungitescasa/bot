#pragma once

#include "api/twitch/helix_client.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "localization/localization.hpp"

namespace bot {
  struct InstanceBundle {
      irc::Client &irc_client;
      const api::twitch::HelixClient &helix_client;
      const bot::loc::Localization &localization;
      const Configuration &configuration;
  };
}
