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
    } else if (type == "7tv_emote_add") {
      return EventType::STV_EMOTE_CREATE;
    } else if (type == "7tv_emote_delete") {
      return EventType::STV_EMOTE_DELETE;
    } else if (type == "7tv_emote_update") {
      return EventType::STV_EMOTE_UPDATE;
    }
#ifdef BUILD_BETTERTTV
    else if (type == "bttv_emote_add") {
      return EventType::BTTV_EMOTE_CREATE;
    } else if (type == "bttv_emote_delete") {
      return EventType::BTTV_EMOTE_DELETE;
    } else if (type == "bttv_emote_update") {
      return EventType::BTTV_EMOTE_UPDATE;
    }
#endif
    else {
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
    } else if (type == STV_EMOTE_CREATE) {
      return "7tv_emote_add";
    } else if (type == STV_EMOTE_DELETE) {
      return "7tv_emote_delete";
    } else if (type == STV_EMOTE_UPDATE) {
      return "7tv_emote_update";
    }
#ifdef BUILD_BETTERTTV
    else if (type == BTTV_EMOTE_CREATE) {
      return "bttv_emote_add";
    } else if (type == BTTV_EMOTE_DELETE) {
      return "bttv_emote_delete";
    } else if (type == BTTV_EMOTE_UPDATE) {
      return "bttv_emote_update";
    }
#endif
    else {
      return "custom";
    }
  }

  std::optional<int> string_to_event_flag(const std::string &type) {
    if (type == "massping") {
      return MASSPING;
    }

    return std::nullopt;
  }

  std::optional<std::string> event_flag_to_string(const int &type) {
    if (type == MASSPING) {
      return "massping";
    }

    return std::nullopt;
  }
}
