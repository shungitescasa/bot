#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <string>
#include <vector>

#include "message.hpp"

namespace bot {
  namespace irc {
    class Client {
      public:
        Client(std::string client_id, std::string token);
        ~Client() = default;

        void run();

        void say(const std::string &channel_login, const std::string &message);
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

        const std::string &get_bot_username() const { return this->username; };
        const int &get_bot_id() const { return this->id; }

      private:
        void authorize();

        std::string client_id, token, username;

        std::string host;
        std::string port;

        int id;

        ix::WebSocket websocket;

        bool is_connected = false;
        std::vector<std::string> pool;

        std::vector<std::string> joined_channels;

        // Message handlers
        typename MessageHandler<MessageType::Privmsg>::fn onPrivmsg;
    };
  }
}
