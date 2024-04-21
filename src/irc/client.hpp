#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <string>
#include <vector>

#include "message.hpp"

namespace bot {
  namespace irc {
    class Client {
      public:
        Client(std::string username, std::string password);
        ~Client() = default;

        void run();

        bool join(const std::string &channel_login);
        void raw(const std::string &raw_message);

        template <MessageType T>
        void on(typename MessageHandler<T>::fn function) {
          switch (T) {
            case Privmsg:
              this->onPrivmsg = function;
              break;
            default:
              break;
          }
        }

      private:
        void authorize();

        std::string username;
        std::string password;

        std::string host;
        std::string port;

        ix::WebSocket websocket;

        bool is_connected = false;
        std::vector<std::string> pool;

        std::vector<std::string> joined_channels;

        // Message handlers
        typename MessageHandler<MessageType::Privmsg>::fn onPrivmsg;
    };
  }
}
