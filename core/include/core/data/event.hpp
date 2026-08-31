#pragma once

#include <string>
#include <vector>

#include "core/data/database.hpp"
#include "core/event.hpp"

namespace bot::data {
  enum class StreamerType { Twitch, Kick };

  struct StreamerData {
      int id;
      StreamerType type;
      bool is_live;
      std::string title;
      std::string game;
  };

  struct Event {
    public:
      Event() = default;
      Event(const DatabaseRow &row);

      std::string create_event_message(const std::string &type,
                                       const std::string &name,
                                       const RSSItem &item);

      int id, room_alias_id;
      std::string message, room_name;
      bool is_massping;
      std::vector<std::string> subs;
  };

  std::vector<Event> get_events(const std::string &type,
                                const std::string &name);
}
