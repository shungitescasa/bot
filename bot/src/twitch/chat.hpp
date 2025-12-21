#pragma once

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

      void start();

      void say(const std::string &channel_login,
               const std::string &message) override;
      void say(unsigned int channel_id, const std::string &message) override;
      void join(const std::string &channel_login) override;
      void join(unsigned int channel_id) override;

      const unsigned int &get_user_id() const override;
      const std::string &get_username() const override;

    private:
      void prepare();
      bool validate_token();
      void handleWebsocketMessage(const std::string &raw);

      ix::WebSocket read_websocket;

      const unsigned int user_id;
      const std::string &client_token, &client_id;
      std::string websocket_session_id, username;

      std::vector<unsigned int> joined_channels;
  };
}