#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace bot::api {
  struct KickChannel {
      int broadcaster_user_id;
      std::string slug, stream_title, stream_game_name;
      bool is_live;
      std::chrono::system_clock::time_point start_time;
  };

  class KickAPIClient {
    public:
      KickAPIClient(const std::string &client_id,
                    const std::string &client_secret)
          : client_id(client_id), client_secret(client_secret) {
        this->authorize();
      };

      ~KickAPIClient() = default;

      std::vector<KickChannel> get_channels(const std::vector<int> &ids) const;
      std::vector<KickChannel> get_channels(const std::string &slug) const;

      void refresh_token_thread();

    private:
      void authorize();

      std::string authorization_key;
      int expires_in;
      long token_acquired_timestamp;
      const std::string base_url = "https://api.kick.com", client_id,
                        client_secret;
  };
}