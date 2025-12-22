#pragma once

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    enum LineId {
      ArgumentSubcommand,
      ArgumentMessage,
      ArgumentName,
      ArgumentTarget,
      ArgumentValue,

      ErrorNotEnoughArguments,
      ErrorIncorrectArgument,
      ErrorExternalAPIError,
      ErrorIllegalCommand,
      ErrorLuaExecutionError
    };

    std::optional<LineId> string_to_line_id(const std::string &str);
  }
}
