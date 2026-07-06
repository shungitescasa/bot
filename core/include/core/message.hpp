#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace bot {
  struct MessageSource {
      std::string login = "";
      unsigned int id = 0;

      MessageSource() = default;
      MessageSource(const std::string &login, const int &id)
          : login(login), id(id) {}
  };

  struct MessageSender {
      std::string login = "", display_name = "";
      unsigned int id = 0;

      bool is_first_message = false;

      std::map<std::string, std::string> badges;
  };

  struct MessageReply {
      std::string id = "", login = "", display_name = "", message = "";
      unsigned int user_id = 0;
  };

  enum class MessageType { ChatMessage, Notification, Connect };

  template <MessageType T>
  struct Message;

  template <>
  struct Message<MessageType::ChatMessage> {
      MessageSender sender;
      MessageSource source;
      std::optional<MessageReply> reply;
      std::string contents = "";

      Message(MessageSender sender, MessageSource source,
              std::optional<MessageReply> reply, std::string contents)
          : sender(sender), source(source), reply(reply), contents(contents) {}
  };

  template <>
  struct Message<MessageType::Notification> {
      std::optional<std::string> reason_id;
      std::string room_name;
      std::string reason;
  };

  template <MessageType T>
  struct MessageHandler;

  template <>
  struct MessageHandler<MessageType::ChatMessage> {
      using fn = std::function<void(Message<MessageType::ChatMessage> message)>;
  };

  template <>
  struct MessageHandler<MessageType::Notification> {
      using fn =
          std::function<void(Message<MessageType::Notification> message)>;
  };

  template <>
  struct MessageHandler<MessageType::Connect> {
      using fn = std::function<void()>;
  };
}