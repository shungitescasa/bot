#pragma once

#include <functional>
#include <string>
#include <utility>

#include "irc/message.hpp"

namespace bot::chat {
  class ChatClient {
    public:
      virtual void say(const std::string &channel_login,
                       const std::string &message) = 0;
      virtual void say(unsigned int channel_id, const std::string &message) = 0;

      virtual void join(const std::string &channel_login) = 0;
      virtual void join(unsigned int channel_id) = 0;

      virtual const std::string &get_username() const = 0;
      virtual const unsigned int &get_user_id() const = 0;
  };

  class EventChatClient {
    public:
      void on_connect(std::function<void()> fn) {
        this->onConnect = std::move(fn);
      }

      void on_privmsg(
          std::function<void(irc::Message<irc::Privmsg> message)> fn) {
        this->onPrivmsg = std::move(fn);
      }

    protected:
      typename irc::MessageHandler<irc::MessageType::Privmsg>::fn onPrivmsg;
      typename irc::MessageHandler<irc::MessageType::Connect>::fn onConnect;
  };
}