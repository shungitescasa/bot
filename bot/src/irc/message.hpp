#pragma once

#include <functional>
#include <map>
#include <optional>
#include <sol/table.hpp>
#include <string>
#include <vector>

#include "../utils/string.hpp"

namespace bot {
  namespace irc {
    enum MessageType { Privmsg, Ping, Notice, Connect };
    std::optional<MessageType> define_message_type(const std::string &msg);

    struct IRCMessage {
        std::map<std::string, std::string> tags;
        std::string prefix, nick, command;
        std::vector<std::string> params;

        static std::optional<IRCMessage> from_string(std::string msg);
    };

    struct MessageSender {
        std::string login;
        std::string display_name;
        int id;

        bool is_first_message = false;

        std::map<std::string, std::string> badges;

        // More fields will be here
    };

    struct MessageSource {
        std::string login;
        int id;

        MessageSource() = default;
        MessageSource(const std::string &login, const int &id);
        MessageSource(const sol::table &table);
    };

    struct MessageReply {
        std::string id, login, display_name, message;
        unsigned int user_id;
    };

    template <MessageType T>
    struct Message;

    template <>
    struct Message<MessageType::Privmsg> {
        MessageSender sender;
        MessageSource source;
        std::optional<MessageReply> reply;
        std::string message;
    };

    template <>
    struct Message<MessageType::Notice> {
        std::optional<std::string> reason_id;
        std::string room_name;
        std::string reason;
    };

    template <MessageType T>
    std::optional<Message<T>> parse_message(const IRCMessage &msg) {
      if constexpr (T == MessageType::Privmsg) {
        if (msg.command != "PRIVMSG") return std::nullopt;

        MessageSender sender;
        MessageSource source;

        sender.login = msg.nick;
        sender.display_name = msg.tags.at("display-name");
        sender.id = std::stoi(msg.tags.at("user-id"));
        for (const std::string &badge :
             utils::string::split_text(msg.tags.at("badges"), ',')) {
          auto b = utils::string::split_text_n(badge, "/", 1);
          sender.badges.insert_or_assign(b[0], b[1]);
        }

        if (msg.tags.count("first-msg")) {
          sender.is_first_message = std::stoi(msg.tags.at("first-msg"));
        }

        source.login = msg.params.front();
        if (!source.login.empty() && source.login[0] == '#') {
          source.login = source.login.substr(1);
        }
        source.id = std::stoi(msg.tags.at("room-id"));

        std::string message_text = msg.params.at(1);

        // searching for reply
        std::optional<MessageReply> reply = std::nullopt;
        if (msg.tags.count("reply-parent-msg-id") &&
            msg.tags.count("reply-parent-user-login") &&
            msg.tags.count("reply-parent-user-id") &&
            msg.tags.count("reply-parent-display-name")) {
          reply = MessageReply{};
          reply->id = msg.tags.at("reply-parent-msg-id");
          reply->login = msg.tags.at("reply-parent-user-login");
          reply->display_name = msg.tags.at("reply-parent-display-name");
          reply->user_id = std::stoi(msg.tags.at("reply-parent-user-id"));
          reply->message = msg.tags.at("reply-parent-msg-body");
          utils::string::replace(reply->message, "\\s", " ");
          utils::string::replace(reply->message, "\\:", ";");

          if (message_text.substr(0, reply->login.length() + 1) ==
              "@" + reply->login) {
            message_text = message_text.substr(reply->login.length() + 2);
          }
        }

        Message<MessageType::Privmsg> message;
        message.sender = sender;
        message.source = source;
        message.reply = reply;
        message.message = message_text;

        return message;
      }

      if constexpr (T == MessageType::Notice) {
        if (msg.command != "NOTICE") return std::nullopt;

        Message<MessageType::Notice> message;

        if (!msg.params.empty()) {
          message.room_name = msg.params.at(0);
        }

        if (msg.params.size() > 1) {
          message.reason = msg.params.at(1);
        }

        if (msg.tags.count("msg-id")) {
          message.reason_id = msg.tags.at("msg-id");
        }

        return message;
      }

      return std::nullopt;
    }

    template <MessageType T>
    struct MessageHandler;

    template <>
    struct MessageHandler<MessageType::Privmsg> {
        using fn = std::function<void(Message<Privmsg> message)>;
    };

    template <>
    struct MessageHandler<MessageType::Notice> {
        using fn = std::function<void(Message<Notice> message)>;
    };

    template <>
    struct MessageHandler<MessageType::Connect> {
        using fn = std::function<void()>;
    };
  }
}
