#pragma once

#include <string>

namespace bot::schemas {
  enum EventType { LIVE, OFFLINE, TITLE, GAME, GITHUB = 10, CUSTOM = 99 };
  EventType string_to_event_type(const std::string &type);
  std::string event_type_to_string(const int &type);

  enum EventFlag { MASSPING };

}
