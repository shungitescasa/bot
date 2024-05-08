#include "line_id.hpp"

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    std::optional<LineId> string_to_line_id(const std::string &str) {
      if (str == "ping.response") {
        return LineId::PingResponse;
      }

      else if (str == "msg.owner") {
        return LineId::MsgOwner;
      }

      else if (str == "argument.subcommand") {
        return LineId::ArgumentSubcommand;
      } else if (str == "argument.message") {
        return LineId::ArgumentMessage;
      } else if (str == "argument.interval") {
        return LineId::ArgumentInterval;
      } else if (str == "argument.name") {
        return LineId::ArgumentName;
      } else if (str == "argument.target") {
        return LineId::ArgumentTarget;
      } else if (str == "argument.value") {
        return LineId::ArgumentValue;
      } else if (str == "argument.amount") {
        return LineId::ArgumentAmount;
      }

      else if (str == "error.template") {
        return LineId::ErrorTemplate;
      } else if (str == "error.not_enough_arguments") {
        return LineId::ErrorNotEnoughArguments;
      } else if (str == "error.incorrect_argument") {
        return LineId::ErrorIncorrectArgument;
      } else if (str == "error.incompatible_name") {
        return LineId::ErrorIncompatibleName;
      } else if (str == "error.namesake_creation") {
        return LineId::ErrorNamesakeCreation;
      } else if (str == "error.not_found") {
        return LineId::ErrorNotFound;
      } else if (str == "error.something_went_wrong") {
        return LineId::ErrorSomethingWentWrong;
      } else if (str == "error.insufficient_rights") {
        return LineId::ErrorInsufficientRights;
      }

      else if (str == "event.on") {
        return LineId::EventOn;
      } else if (str == "event.off") {
        return LineId::EventOff;
      }

      else if (str == "notify.sub") {
        return LineId::NotifySub;
      } else if (str == "notify.unsub") {
        return LineId::NotifyUnsub;
      }

      else if (str == "join.response") {
        return LineId::JoinResponse;
      } else if (str == "join.response_in_chat") {
        return LineId::JoinResponseInChat;
      } else if (str == "join.already_in") {
        return LineId::JoinAlreadyIn;
      } else if (str == "join.rejoined") {
        return LineId::JoinRejoined;
      } else if (str == "join.from_other_chat") {
        return LineId::JoinFromOtherChat;
      } else if (str == "join.not_allowed") {
        return LineId::JoinNotAllowed;
      }

      else {
        return std::nullopt;
      }
    }
  }
}
