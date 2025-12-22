#include "line_id.hpp"

#include <optional>
#include <string>

namespace bot {
  namespace loc {
    std::optional<LineId> string_to_line_id(const std::string &str) {
      if (str == "argument.subcommand") {
        return LineId::ArgumentSubcommand;
      } else if (str == "argument.message") {
        return LineId::ArgumentMessage;
      } else if (str == "argument.name") {
        return LineId::ArgumentName;
      } else if (str == "argument.target") {
        return LineId::ArgumentTarget;
      } else if (str == "argument.value") {
        return LineId::ArgumentValue;
      }

      else if (str == "error.not_enough_arguments") {
        return LineId::ErrorNotEnoughArguments;
      } else if (str == "error.incorrect_argument") {
        return LineId::ErrorIncorrectArgument;
      } else if (str == "error.external_api_error") {
        return LineId::ErrorExternalAPIError;
      } else if (str == "error.illegal_command") {
        return LineId::ErrorIllegalCommand;
      } else if (str == "error.lua_execution_error") {
        return LineId::ErrorLuaExecutionError;
      }

      else {
        return std::nullopt;
      }
    }
  }
}
