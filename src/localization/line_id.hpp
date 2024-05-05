#pragma once

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    enum LineId {
      ErrorTemplate,
      ErrorNotEnoughArguments,
      ErrorIncorrectArgument,
      ErrorIncompatibleName,
      ErrorNamesakeCreation,
      ErrorNotFound,
      ErrorSomethingWentWrong,
      ErrorInsufficientRights,

      PingResponse
    };

    std::optional<LineId> string_to_line_id(const std::string &str);
  }
}
