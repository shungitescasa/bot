#pragma once

#include <set>
#include <vector>

#include "api/twitch/helix_client.hpp"
#include "api/twitch/schemas/stream.hpp"
#include "config.hpp"
#include "irc/client.hpp"
#include "schemas/stream.hpp"

namespace bot::stream {
  class StreamListenerClient {
    public:
      StreamListenerClient(const api::twitch::HelixClient &helix_client,
                           irc::Client &irc_client,
                           const Configuration &configuration)
          : helix_client(helix_client),
            irc_client(irc_client),
            configuration(configuration){};
      ~StreamListenerClient() = default;

      void run_thread();

      void listen_channel(const int &id);
      void unlisten_channel(const int &id);

    private:
      void run();
      void check();
      void handler(const schemas::EventType &type,
                   const api::twitch::schemas::Stream &stream);
      void update_channel_ids();

      const api::twitch::HelixClient &helix_client;
      irc::Client &irc_client;
      const Configuration &configuration;

      std::vector<int> ids;

      std::set<int> online_ids;
  };
}
