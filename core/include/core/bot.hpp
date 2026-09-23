#pragma once

#include <algorithm>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/externalapi/twitch.hpp"
#include "core/log.hpp"
#include "core/message.hpp"
#include "rpc/client.h"
#include "rpc/server.h"

namespace bot {
  class ChatBot {
    public:
      ChatBot() = default;
      ~ChatBot() = default;

      virtual void send_message(const MessageSource &source,
                                const std::string &message) = 0;

      virtual void connect() = 0;

      virtual void join(const MessageSource &source) = 0;
      virtual void part(const MessageSource &source) = 0;

      virtual const MessageSource &get_me() const = 0;

      virtual void ping_server() = 0;

      bool has_already_joined(const MessageSource &source) {
        return std::any_of(this->joined_rooms.begin(), this->joined_rooms.end(),
                           [&source](const MessageSource &s) {
                             return s.normalize() == source.normalize() ||
                                    (s.id != 0 && s.id == source.id);
                           });
      }

      int room_count() const { return this->joined_rooms.size(); }

    protected:
      std::vector<MessageSource> joined_rooms;
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

  class RPCChatBotServer {
    public:
      RPCChatBotServer(std::shared_ptr<ChatBot> bot, unsigned int port)
          : bot(bot),
            server(port),
            log(std::format("RPCChatBotServer:{}", port)) {}
      ~RPCChatBotServer() = default;

      void run();

    private:
      bot::Logger log;
      rpc::server server;
      std::shared_ptr<ChatBot> bot;
  };

  class RPCChatBot {
    public:
      RPCChatBot() = default;
      RPCChatBot(std::string host, unsigned int port, unsigned int timeout = 0)
          : host(std::move(host)),
            port(port),
            timeout(timeout),
            log(std::format("TinyBot-RPCClient/{}:{}", host, port)) {
        connect();
      }
      RPCChatBot(const RPCChatBot &) = delete;
      RPCChatBot &operator=(const RPCChatBot &) = delete;

      bool is_alive();

      bool connect();
      bool connect(std::string host, unsigned int port,
                   unsigned int timeout = 0);

      void send_message(const MessageSource &source,
                        const std::string &message);
      void join(const MessageSource &source);
      void part(const MessageSource &source);

      MessageSource get_me();

      // --- Twitch API

      std::vector<externalapi::twitch::User> get_chatters(
          const int &broadcaster_id);
      std::vector<externalapi::twitch::User> get_users(
          const std::vector<int> &ids, const std::vector<std::string> &logins);
      std::vector<externalapi::twitch::MsgPackEmote> get_global_emotes();
      std::vector<externalapi::twitch::MsgPackEmote> get_channel_emotes(
          const int &id);

      static RPCChatBot &get_instance() {
        static RPCChatBot instance;
        return instance;
      }

    private:
      bot::Logger log;
      std::string host;
      unsigned int port, timeout;
      std::unique_ptr<rpc::client> client;
  };
}
