#pragma once

#include "chat.hpp"
#include "config.hpp"

namespace bot {
  void create_timer_thread(chat::ChatClient *irc_client,
                           Configuration *configuration);
}
