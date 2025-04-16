#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "api/twitch/helix_client.hpp"
#include "config.hpp"
#include "irc/client.hpp"

namespace bot {
  struct Commit {
      std::string sha;
      std::string commiter_name;
      std::string message;
  };

  class GithubListener {
    public:
      GithubListener(const Configuration &configuration,
                     irc::Client &irc_client,
                     const api::twitch::HelixClient &helix_client)
          : configuration(configuration),
            irc_client(irc_client),
            helix_client(helix_client) {};
      ~GithubListener() {};

      void run();

    private:
      void check_for_listeners();
      std::unordered_map<std::string, std::vector<Commit>> check_new_commits();
      void notify_about_commits(
          const std::unordered_map<std::string, std::vector<Commit>>
              &new_commits);

      std::vector<std::string> ids;
      std::unordered_map<std::string, std::vector<std::string>> commits;

      irc::Client &irc_client;
      const Configuration &configuration;
      const api::twitch::HelixClient &helix_client;
  };
}