#pragma once

#include <set>
#include <vector>

#include "api/kick.hpp"
#include "api/twitch/helix_client.hpp"
#include "api/twitch/schemas/stream.hpp"
#include "chat.hpp"
#include "config.hpp"
#include "schemas/stream.hpp"

namespace bot::stream {
  enum StreamerType { TWITCH, KICK };

  struct StreamerData {
      int id;
      StreamerType type;
      bool is_live;
      std::string title;
      std::string game;
  };

  class StreamListenerClient {
    public:
      StreamListenerClient(const api::twitch::HelixClient &helix_client,
                           const api::KickAPIClient &kick_api_client,
                           chat::ChatClient &irc_client)
          : helix_client(helix_client),
            kick_api_client(kick_api_client),
            irc_client(irc_client){};
      ~StreamListenerClient() = default;

      void run();
      void listen_channel(const int &id, const StreamerType &type);
      void unlisten_channel(const int &id, const StreamerType &type);

    private:
      void check();
      void handler(const schemas::EventType &type,
                   const api::twitch::schemas::Stream &stream,
                   const StreamerData &data);
      void update_channel_ids();

      const api::twitch::HelixClient &helix_client;
      const api::KickAPIClient &kick_api_client;
      chat::ChatClient &irc_client;

      std::vector<StreamerData> streamers;
  };
}
