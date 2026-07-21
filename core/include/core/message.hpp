#pragma once

#include <functional>
#include <map>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <utility>

#include "rpc/msgpack.hpp"

namespace bot {
  struct MessageSource {
      std::string login = "";
      unsigned int id = 0;

      MSGPACK_DEFINE(login, id);

      MessageSource() = default;
      MessageSource(const std::string &login)
          : login(std::move(login)), id(0) {}
      MessageSource(const unsigned int &id) : login(""), id(id) {}
      MessageSource(const std::string &login, const unsigned int &id)
          : login(std::move(login)), id(id) {}
      MessageSource(const sol::table &table) {
        if (table["login"].valid()) this->login = table["login"];
        if (table["id"].valid()) this->id = table["id"];
      }

      std::string normalize() const {
        if (login.starts_with("#")) return login.substr(1);
        return login;
      }

      std::string unnormalize() const {
        if (!login.starts_with("#")) return "#" + login;
        return login;
      }
  };

  struct MessageSender {
      std::string login = "", display_name = "";
      unsigned int id = 0;

      bool is_first_message = false;

      std::map<std::string, std::string> badges;

      MSGPACK_DEFINE(login, display_name, id, is_first_message, badges);
  };

  struct MessageReply {
      std::string id = "", login = "", display_name = "", message = "";
      unsigned int user_id = 0;

      MSGPACK_DEFINE(id, login, display_name, message, user_id);
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
