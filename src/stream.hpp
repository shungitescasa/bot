#pragma once

#include <set>
#include <vector>

#include "api/twitch/helix_client.hpp"

namespace bot::stream {
  class StreamListenerClient {
    public:
      StreamListenerClient(const api::twitch::HelixClient &helix_client)
          : helix_client(helix_client){};
      ~StreamListenerClient() = default;

      void run_thread();

      void listen_channel(const int &id);
      void unlisten_channel(const int &id);

    private:
      void run();
      void check();

      const api::twitch::HelixClient &helix_client;

      std::vector<int> ids;

      std::set<int> online_ids;
  };
}
