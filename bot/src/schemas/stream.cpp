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

  std::string event_type_to_string(const int &type) {
    if (type == LIVE) {
      return "live";
    } else if (type == OFFLINE) {
      return "offline";
    } else if (type == TITLE) {
      return "title";
    } else if (type == GAME) {
      return "game";
    } else if (type == GITHUB) {
      return "github";
    } else {
      return "custom";
    }
  }
}
