#pragma once

#include "config.hpp"
#include "irc/client.hpp"

namespace bot {
  void create_timer_thread(irc::Client *irc_client,
                           Configuration *configuration);
}
