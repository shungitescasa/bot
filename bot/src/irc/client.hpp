#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <string>
#include <vector>

#include "chat.hpp"
#include "message.hpp"

namespace bot {
  namespace irc {
    class Client : public chat::ChatClient {
      public:
        Client(std::string client_id, std::string token);
        ~Client() = default;

        void run();

        void say(const std::string &channel_login,
                 const std::string &message) override;
        void join(const std::string &channel_login) override;
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

        const std::string &get_username() const override {
          return this->username;
        };
        const unsigned int &get_user_id() const override { return this->id; }

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
