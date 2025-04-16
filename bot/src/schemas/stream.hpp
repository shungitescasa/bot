#pragma once

#include <optional>
#include <string>

namespace bot::schemas {
  enum EventType { LIVE, OFFLINE, TITLE, GAME, GITHUB = 10, CUSTOM = 99 };
  EventType string_to_event_type(const std::string &type);
  std::string event_type_to_string(const int &type);

  enum EventFlag { MASSPING };
  std::optional<int> string_to_event_flag(const std::string &type);
  std::optional<std::string> event_flag_to_string(const int &type);

}
