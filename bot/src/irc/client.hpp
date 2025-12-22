#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <string>
#include <vector>

#include "chat.hpp"
#include "message.hpp"

namespace bot {
  namespace irc {
    class Client : public chat::ChatClient, public chat::EventChatClient {
      public:
        Client(std::string client_id, std::string token);
        ~Client() = default;

        void run();

        void say(const irc::MessageSource &room,
                 const std::string &message) override;
        void join(const irc::MessageSource &room) override;
        void raw(const std::string &raw_message);

      private:
        void authorize();

        std::string client_id, token;

        std::string host;
        std::string port;

        ix::WebSocket websocket;

        bool is_connected = false;
        std::vector<std::string> pool;

        std::vector<std::string> joined_channels;
    };
  }
}
