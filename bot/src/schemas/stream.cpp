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
    } else if (type == "kick_live") {
      return EventType::KICK_LIVE;
    } else if (type == "kick_offline") {
      return EventType::KICK_OFFLINE;
    } else if (type == "kick_title") {
      return EventType::KICK_TITLE;
    } else if (type == "kick_game") {
      return EventType::KICK_GAME;
    } else if (type == "github") {
      return EventType::GITHUB;
    } else if (type == "7tv_new_emote") {
      return EventType::STV_EMOTE_CREATE;
    } else if (type == "7tv_deleted_emote") {
      return EventType::STV_EMOTE_DELETE;
    } else if (type == "7tv_updated_emote") {
      return EventType::STV_EMOTE_UPDATE;
    }
#ifdef BUILD_BETTERTTV
    else if (type == "bttv_new_emote") {
      return EventType::BTTV_EMOTE_CREATE;
    } else if (type == "bttv_deleted_emote") {
      return EventType::BTTV_EMOTE_DELETE;
    } else if (type == "bttv_updated_emote") {
      return EventType::BTTV_EMOTE_UPDATE;
    }
#endif
    else if (type == "rss") {
      return EventType::RSS;
    } else if (type == "twitter") {
      return EventType::TWITTER;
    } else if (type == "telegram") {
      return EventType::TELEGRAM;
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
    } else if (type == KICK_LIVE) {
      return "kick_live";
    } else if (type == KICK_OFFLINE) {
      return "kick_offline";
    } else if (type == KICK_TITLE) {
      return "kick_title";
    } else if (type == KICK_GAME) {
      return "kick_game";
    } else if (type == GITHUB) {
      return "github";
    } else if (type == STV_EMOTE_CREATE) {
      return "7tv_new_emote";
    } else if (type == STV_EMOTE_DELETE) {
      return "7tv_deleted_emote";
    } else if (type == STV_EMOTE_UPDATE) {
      return "7tv_updated_emote";
    }
#ifdef BUILD_BETTERTTV
    else if (type == BTTV_EMOTE_CREATE) {
      return "bttv_new_emote";
    } else if (type == BTTV_EMOTE_DELETE) {
      return "bttv_deleted_emote";
    } else if (type == BTTV_EMOTE_UPDATE) {
      return "bttv_updated_emote";
    }
#endif
    else if (type == RSS) {
      return "rss";
    } else if (type == TWITTER) {
      return "twitter";
    } else if (type == TELEGRAM) {
      return "telegram";
    } else {
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
