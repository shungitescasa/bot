#include "line_id.hpp"

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    std::optional<LineId> string_to_line_id(const std::string &str) {
      if (str == "ping.response") {
        return LineId::PingResponse;
      } else if (str == "error.template") {
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
      } else {
        return std::nullopt;
      }
    }
  }
}
