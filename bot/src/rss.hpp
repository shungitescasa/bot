#pragma once

#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <vector>

#include "api/twitch/helix_client.hpp"
#include "config.hpp"
#include "irc/client.hpp"

namespace bot {
  struct RSSMessage {
      std::string title, id, message;
      long timestamp;
  };

  struct RSSEvent {
      std::string name;
      int type;
  };

  struct RSSChannel {
      std::string name, url;
      std::optional<RSSEvent> event;
      std::vector<RSSMessage> messages;

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const;
  };

  class RSSListener {
    public:
      RSSListener(irc::Client &irc_client,
                  api::twitch::HelixClient &helix_client,
                  Configuration &configuration)
          : irc_client(irc_client),
            helix_client(helix_client),
            configuration(configuration) {};
      ~RSSListener() = default;

      void run();
      void add_channel(const std::string &url);
      void remove_channel(const std::string &url);
      bool has_channel(const std::string &url) const;

    private:
      void add_channels();
      void check_channels();

      std::vector<RSSChannel> channels;
      irc::Client &irc_client;
      api::twitch::HelixClient &helix_client;
      Configuration &configuration;
  };

  std::optional<RSSChannel> get_rss_channel(const std::string &url);
}