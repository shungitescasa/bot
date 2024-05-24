#pragma once

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    enum LineId {
      MsgOwner,

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
      ErrorIllegalCommand,

      PingResponse,

      EventOn,
      EventOff,

      NotifySub,
      NotifyUnsub,

      JoinResponse,
      JoinResponseInChat,
      JoinAlreadyIn,
      JoinRejoined,
      JoinFromOtherChat,
      JoinNotAllowed,

      CustomcommandNew,
      CustomcommandDelete,

      TimerNew,
      TimerDelete,

      HelpResponse,

      ChattersResponse
    };

    std::optional<LineId> string_to_line_id(const std::string &str);
  }
}
