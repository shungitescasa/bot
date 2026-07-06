#pragma once

#include <functional>
#include <string>

#include "core/message.hpp"

namespace bot {
  class ChatBot {
    public:
      ChatBot() = default;
      ~ChatBot() = default;

      virtual void send_message(const std::string &room,
                                const std::string &message) = 0;

      virtual void connect() = 0;
  };

  class EventChatBot {
    public:
      void on_connect(std::function<void()> fn) {
        this->onConnect = std::move(fn);
      }

      void on_chat_message(
          std::function<void(Message<MessageType::ChatMessage> message)> fn) {
        this->onChatMessage = std::move(fn);
      }

      void on_notification(
          std::function<void(Message<MessageType::Notification> message)> fn) {
        this->onNotification = std::move(fn);
      }

    protected:
      typename MessageHandler<MessageType::ChatMessage>::fn onChatMessage;
      typename MessageHandler<MessageType::Connect>::fn onConnect;
      typename MessageHandler<MessageType::Notification>::fn onNotification;
  };
}