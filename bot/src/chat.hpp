#pragma once

#include <functional>
#include <string>
#include <utility>

#include "irc/message.hpp"

namespace bot::chat {
  class ChatClient {
    public:
      virtual void say(const irc::MessageSource &room,
                       const std::string &message) = 0;
      virtual void join(const irc::MessageSource &room) = 0;

      irc::MessageSource &get_me() { return this->me; };

    protected:
      irc::MessageSource me;
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