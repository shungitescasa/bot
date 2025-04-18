#pragma once

#include <emotespp/seventv.hpp>
#include <optional>
#include <string>

#include "api/twitch/helix_client.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "schemas/stream.hpp"

namespace bot::emotes {
  struct EmoteEventBundle {
      irc::Client &irc_client;
      const api::twitch::HelixClient &helix_client;
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