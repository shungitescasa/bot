#pragma once

#include <optional>
#include <string>

namespace bot::schemas {
  enum EventType {
    LIVE,
    OFFLINE,
    TITLE,
    GAME,
    KICK_LIVE,
    KICK_OFFLINE,
    KICK_TITLE,
    KICK_GAME,
    STV_EMOTE_CREATE = 10,
    STV_EMOTE_DELETE,
    STV_EMOTE_UPDATE,
#ifdef BUILD_BETTERTTV
    BTTV_EMOTE_CREATE,
    BTTV_EMOTE_DELETE,
    BTTV_EMOTE_UPDATE,
#endif
    GITHUB = 40,
    RSS = 45,
    TWITTER = 46,
    TELEGRAM = 47,
    CUSTOM = 99
  };

  EventType string_to_event_type(const std::string &type);
  std::string event_type_to_string(const int &type);

  enum EventFlag { MASSPING };
  std::optional<int> string_to_event_flag(const std::string &type);
  std::optional<std::string> event_flag_to_string(const int &type);

}
