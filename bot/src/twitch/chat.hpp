#pragma once
#include <optional>
#include <unordered_map>

#include "config.hpp"
#ifdef USE_EVENTSUB_CONNECTION

#include <string>
#include <vector>

#include "../chat.hpp"
#include "ixwebsocket/IXWebSocket.h"

namespace bot::twitch {
  class TwitchChatClient : public chat::EventChatClient,
                           public chat::ChatClient {
    public:
      TwitchChatClient(const TwitchConfiguration &cfg)
          : user_id(cfg.user_id),
            user_token(cfg.user_token),
            user_client_id(cfg.user_client_id),
            app_client_id(cfg.app_client_id),
            app_client_secret(cfg.app_client_secret) {
        prepare();
      }

      ~TwitchChatClient() = default;

      void run();

      void say(const irc::MessageSource &room,
               const std::string &message) override;
      void join(const irc::MessageSource &room) override;
      void part(const irc::MessageSource &room) override;

    private:
      void prepare();
      bool validate_token();
      void authorize_app();
      void handleWebsocketMessage(const std::string &raw);

      ix::WebSocket read_websocket;

      const unsigned int user_id;
      const std::string &user_token, &user_client_id, &app_client_id,
          &app_client_secret;
      std::string app_token;
      unsigned int app_token_expiration, app_token_acquisition;
      std::string websocket_session_id;

      std::unordered_map<unsigned int, std::optional<std::string>>
          joined_channels;
  };
}
#endif