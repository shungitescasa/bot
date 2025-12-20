#pragma once

#include <emotespp/betterttv.hpp>
#include <emotespp/seventv.hpp>
#include <optional>
#include <string>

#include "api/twitch/helix_client.hpp"
#include "chat.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "schemas/stream.hpp"

namespace bot::emotes {
  struct EmoteEventBundle {
      chat::ChatClient &irc_client;
      const api::twitch::HelixClient &helix_client;
#ifdef BUILD_BETTERTTV
      emotespp::BetterTTVWebsocketClient &bttv_ws_client;
#endif
      emotespp::SevenTVWebsocketClient &stv_ws_client;
      const emotespp::SevenTVAPIClient &stv_api_client;
      const Configuration &configuration;
  };

  void handle_emote_event(const EmoteEventBundle &bundle,
                          const schemas::EventType &event_type,
                          const std::string &channel_name,
                          const std::optional<std::string> &author_id,
                          const emotespp::Emote &emote);

  void create_emote_thread(const EmoteEventBundle *bundle);
}