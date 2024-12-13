#include "stream.hpp"

namespace bot::schemas {
  EventType string_to_event_type(const std::string &type) {
    if (type == "live") {
      return EventType::LIVE;
    } else if (type == "offline") {
      return EventType::OFFLINE;
    } else if (type == "title") {
      return EventType::TITLE;
    } else if (type == "game") {
      return EventType::GAME;
    } else if (type == "github") {
      return EventType::GITHUB;
    } else {
      return EventType::CUSTOM;
    }
  }
}
