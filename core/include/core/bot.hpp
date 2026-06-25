#pragma once

#include <string>
namespace bot {
  class ChatBot {
    public:
      ChatBot() = default;
      ~ChatBot() = default;

      virtual void send_message(const std::string &room,
                                const std::string &message) = 0;

      virtual void connect() = 0;
  };
}