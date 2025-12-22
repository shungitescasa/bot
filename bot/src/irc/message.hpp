#pragma once

#include <functional>
#include <map>
#include <optional>
#include <sol/table.hpp>
#include <sstream>
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

    template <MessageType T>
    struct Message;

    template <>
    struct Message<MessageType::Privmsg> {
        MessageSender sender;
        MessageSource source;
        std::string message;
    };

    template <MessageType T>
    std::optional<Message<T>> parse_message(const IRCMessage &msg) {
      if (T == MessageType::Privmsg && msg.command == "PRIVMSG") {
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

        source.login = msg.params.front();
        if (source.login[0] == '#') {
          source.login = source.login.substr(1);
        }
        source.id = std::stoi(msg.tags.at("room-id"));

        Message<MessageType::Privmsg> message;
        message.sender = sender;
        message.source = source;
        message.message = msg.params.at(1);

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
    struct MessageHandler<MessageType::Connect> {
        using fn = std::function<void()>;
    };
  }
}
