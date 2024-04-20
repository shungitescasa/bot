#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <string>

#include "message.hpp"

namespace RedpilledBot {
  namespace IRC {
    class Client {
      public:
        Client(std::string username, std::string password);
        ~Client() = default;

        void run();

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

        // Message handlers
        typename MessageHandler<MessageType::Privmsg>::fn onPrivmsg;
    };
  }
}
