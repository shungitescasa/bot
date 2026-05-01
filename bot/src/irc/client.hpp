#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <boost/url.hpp>
#include <optional>
#include <string>
#include <vector>

#include "chat.hpp"
#include "message.hpp"

namespace bot {
  namespace irc {
    class Client : public chat::ChatClient, public chat::EventChatClient {
      public:
        Client(std::string host, std::string client_id, std::string token,
               std::optional<std::string> http_password);
        ~Client() = default;

        void run();

        void say(const irc::MessageSource &room,
                 const std::string &message) override;
        void join(const irc::MessageSource &room) override;
        void part(const irc::MessageSource &room) override;
        void raw(const std::string &raw_message);

      private:
        void authorize();

        std::string client_id, token;

        boost::urls::url_view host;
        std::string password;

        ix::WebSocket websocket;

        bool is_connected = false;
        std::vector<std::string> pool;

        std::vector<std::string> joined_channels;
    };
  }
}
