#pragma once

#include <string>

namespace bot::schemas {
  enum EventType { LIVE, OFFLINE, TITLE, GAME, CUSTOM = 99 };
  EventType string_to_event_type(const std::string &type);

  enum EventFlag { MASSPING };

}
