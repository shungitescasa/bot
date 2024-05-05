#pragma once

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    enum LineId {
      ArgumentSubcommand,
      ArgumentMessage,
      ArgumentInterval,
      ArgumentName,
      ArgumentTarget,
      ArgumentValue,
      ArgumentAmount,

      ErrorTemplate,
      ErrorNotEnoughArguments,
      ErrorIncorrectArgument,
      ErrorIncompatibleName,
      ErrorNamesakeCreation,
      ErrorNotFound,
      ErrorSomethingWentWrong,
      ErrorExternalAPIError,
      ErrorInsufficientRights,

      PingResponse,

      EventOn,
      EventOff,

      NotifySub,
      NotifyUnsub
    };

    std::optional<LineId> string_to_line_id(const std::string &str);
  }
}
