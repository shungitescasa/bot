#include "line_id.hpp"

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    std::optional<LineId> string_to_line_id(const std::string &str) {
      if (str == "ping.response") {
        return LineId::PingResponse;
      } else {
        return std::nullopt;
      }
    }
  }
}
