#pragma once
#ifdef USE_EVENTSUB_CONNECTION

#include <string>
#include <vector>

#include "../chat.hpp"
#include "ixwebsocket/IXWebSocket.h"

namespace bot::twitch {
  class TwitchChatClient : public chat::EventChatClient,
                           public chat::ChatClient {
    public:
      TwitchChatClient(const unsigned int user_id,
                       const std::string &client_token,
                       const std::string &client_id)
          : user_id(user_id), client_token(client_token), client_id(client_id) {
        prepare();
      }

      ~TwitchChatClient() = default;

      void run();

      void say(const irc::MessageSource &room,
               const std::string &message) override;
      void join(const irc::MessageSource &room) override;

    private:
      void prepare();
      bool validate_token();
      void handleWebsocketMessage(const std::string &raw);

      ix::WebSocket read_websocket;

      const unsigned int user_id;
      const std::string &client_token, &client_id;
      std::string websocket_session_id;

      std::vector<unsigned int> joined_channels;
  };
}
#endif