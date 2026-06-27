#include "core/irc/message.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace bot::irc {
  std::optional<IRCMessage> IRCMessage::from(std::string_view line) {
    IRCMessage msg;

    if (line.empty()) return std::nullopt;

    auto trim_front = [&] {
      while (!line.empty() && line.front() == ' ') line.remove_prefix(1);
    };

    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
      line.remove_suffix(1);

    if (line.starts_with('@')) {
      auto space = line.find(' ');
      if (space == std::string_view::npos) return std::nullopt;

      auto tag_list = line.substr(1, space - 1);

      while (!tag_list.empty()) {
        auto semi = tag_list.find(';');
        auto tag = tag_list.substr(0, semi);

        auto eq = tag.find('=');
        if (eq == std::string_view::npos)
          msg.tags.emplace(tag, "");
        else
          msg.tags.emplace(tag.substr(0, eq), tag.substr(eq + 1));

        if (semi == std::string_view::npos) break;

        tag_list.remove_prefix(semi + 1);
      }

      line.remove_prefix(space);
      trim_front();
    }

    if (line.starts_with(':')) {
      line.remove_prefix(1);

      auto space = line.find(' ');
      if (space == std::string_view::npos) return std::nullopt;

      auto prefix = line.substr(0, space);
      msg.prefix = prefix;

      auto excl = prefix.find('!');
      msg.nick = excl == std::string_view::npos
                     ? std::string(prefix)
                     : std::string(prefix.substr(0, excl));

      line.remove_prefix(space);
      trim_front();
    }

    {
      auto space = line.find(" ");
      if (space == std::string::npos) {
        if (line.empty()) return std::nullopt;
        msg.command = line;
        return msg;
      }

      msg.command = line.substr(0, space);
      line.remove_prefix(space);
      trim_front();
    }

    while (!line.empty()) {
      if (line.starts_with(':')) {
        msg.params.emplace_back(line.substr(1));
        break;
      }

      auto space = line.find(' ');
      if (space == std::string_view::npos) {
        msg.params.emplace_back(line);
        break;
      }

      msg.params.emplace_back(line.substr(0, space));
      line.remove_prefix(space);
      trim_front();
    }

    return msg;
  }
}