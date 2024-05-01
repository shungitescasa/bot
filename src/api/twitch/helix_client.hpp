#pragma once

#include <string>
#include <vector>

#include "schemas/user.hpp"

namespace bot::api::twitch {
  class HelixClient {
    public:
      HelixClient(const std::string &token, const std::string &client_id);
      ~HelixClient() = default;

      std::vector<schemas::User> get_users(
          const std::vector<std::string> &logins) const;
      std::vector<schemas::User> get_users(const std::vector<int> &ids) const;

      std::vector<schemas::User> get_chatters(const int &broadcaster_id,
                                              const int &moderator_id) const;

    private:
      std::vector<schemas::User> get_users_by_query(
          const std::string &query) const;
      std::string token, client_id;
      const std::string base_url = "https://api.twitch.tv/helix";
  };
}
