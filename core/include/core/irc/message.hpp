#pragma once

#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/message.hpp"
#include "core/utils.hpp"

namespace bot::irc {
  struct IRCMessage {
      std::unordered_map<std::string, std::string> tags;
      std::string prefix, nick, command;
      std::vector<std::string> params;

      static std::optional<IRCMessage> from(std::string_view line);

      template <MessageType T>
      std::optional<Message<T>> as_message() const {
        if constexpr (T == MessageType::ChatMessage) {
          if (command != "PRIVMSG") return std::nullopt;

          // parsing source
          MessageSource source;
          if (params.size() != 2) return std::nullopt;

          source.login = params.front();
          if (source.login.starts_with("#")) {
            source.login = source.login.substr(1);
          }

          if (tags.contains("room-id")) {
            source.id = std::stoi(tags.at("room-id"));
          }

          // parsing sender
          MessageSender sender;

          sender.login = nick;
          if (tags.contains("display-name")) {
            sender.display_name = tags.at("display-name");
          }

          if (tags.contains("user-id")) {
            sender.id = std::stoi(tags.at("user-id"));
          }

          if (tags.contains("badges")) {
            for (const auto &part :
                 std::ranges::views::split(tags.at("badges"), ',')) {
              std::string_view badge(part.begin(), part.end());
              int pos = badge.find("/");
              std::string k(badge), v = "0";
              if (pos != std::string::npos) {
                k = badge.substr(0, pos);
                v = badge.substr(pos + 1);
              }
              sender.badges.insert_or_assign(k, v);
            }
          }

          if (tags.contains("first-msg")) {
            sender.is_first_message = std::stoi(tags.at("first-msg"));
          }

          // message
          std::string contents = params.back();

          // parsing reply
          std::optional<MessageReply> reply = std::nullopt;
          if (tags.contains("reply-parent-msg-id") &&
              tags.contains("reply-parent-user-login") &&
              tags.contains("reply-parent-user-id") &&
              tags.contains("reply-parent-display-name")) {
            reply = MessageReply{};
            reply->id = tags.at("reply-parent-msg-id");
            reply->login = tags.at("reply-parent-user-login");
            reply->display_name = tags.at("reply-parent-display-name");
            reply->user_id = std::stoi(tags.at("reply-parent-user-id"));

            if (tags.contains("reply-parent-msg-body")) {
              reply->message = tags.at("reply-parent-msg-body");

              utils::string::replace(reply->message, "\\s", " ");
              utils::string::replace(reply->message, "\\:", ";");
            }

            if (contents.starts_with("@" + reply->login)) {
              contents = contents.substr(reply->login.length() + 2);
            }
          }

          return Message<MessageType::ChatMessage>{sender, source, reply,
                                                   contents};
        }

        else if constexpr (T == MessageType::Notification) {
          if (command != "NOTICE") return std::nullopt;

          Message<MessageType::Notification> m;

          if (!params.empty()) {
            m.room_name = params.at(0);
          }

          if (params.size() > 1) {
            m.reason = params.at(1);
          }

          if (tags.count("msg-id")) {
            m.reason_id = tags.at("msg-id");
          }

          return m;
        }

        return std::nullopt;
      };
  };
}