#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace bot::irc {
  struct IRCMessage {
      std::unordered_map<std::string, std::string> tags;
      std::string prefix, nick, command;
      std::vector<std::string> params;

      static std::optional<IRCMessage> from(std::string_view line);
  };
}