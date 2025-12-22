#pragma once

#include <exception>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "command.hpp"
#include "localization/line_id.hpp"
#include "request.hpp"

namespace bot {
  enum ResponseError {
    NOT_ENOUGH_ARGUMENTS,
    INCORRECT_ARGUMENT,

    EXTERNAL_API_ERROR,

    ILLEGAL_COMMAND,

    LUA_EXECUTION_ERROR
  };

  template <ResponseError T, class Enable = void>
  class ResponseException;

  template <ResponseError T>
  class ResponseException<
      T, typename std::enable_if<T == INCORRECT_ARGUMENT ||
                                 T == LUA_EXECUTION_ERROR>::type>
      : public std::exception {
    public:
      ResponseException(const command::Request &request,
                        const loc::Localization &localizator,
                        const std::string &message)
          : request(request),
            localizator(localizator),
            message(message),
            error(T) {
        loc::LineId line_id;

        if (this->error == INCORRECT_ARGUMENT) {
          line_id = loc::LineId::ErrorIncorrectArgument;
        } else {
          line_id = loc::LineId::ErrorLuaExecutionError;
        }

        this->line =
            this->localizator
                .get_formatted_line(this->request, line_id, {this->message})
                .value();
      }
      ~ResponseException() = default;

      const char *what() const noexcept override { return this->line.c_str(); }

    private:
      const command::Request &request;
      const loc::Localization &localizator;
      std::string message, line;
      ResponseError error;
  };

  template <ResponseError T>
  class ResponseException<T,
                          typename std::enable_if<T == ILLEGAL_COMMAND>::type>
      : public std::exception {
    public:
      ResponseException(const command::Request &request,
                        const loc::Localization &localizator)
          : request(request), localizator(localizator), error(T) {
        loc::LineId line_id = loc::LineId::ErrorIllegalCommand;

        this->line =
            this->localizator.get_formatted_line(this->request, line_id, {})
                .value();
      }
      ~ResponseException() = default;

      const char *what() const noexcept override { return this->line.c_str(); }

    private:
      const command::Request &request;
      const loc::Localization &localizator;
      std::string line;
      ResponseError error;
  };

  template <ResponseError T>
  class ResponseException<
      T, typename std::enable_if<T == EXTERNAL_API_ERROR>::type>
      : public std::exception {
    public:
      ResponseException(
          const command::Request &request, const loc::Localization &localizator,
          const int &code,
          const std::optional<std::string> &message = std::nullopt)
          : request(request),
            localizator(localizator),
            code(code),
            message(message),
            error(T) {
        loc::LineId line_id = loc::LineId::ErrorExternalAPIError;
        std::vector<std::string> args = {std::to_string(this->code)};

        if (this->message.has_value()) {
          args.push_back(" " + this->message.value());
        } else {
          args.push_back("");
        }

        this->line =
            this->localizator.get_formatted_line(this->request, line_id, args)
                .value();
      }
      ~ResponseException() = default;

      const char *what() const noexcept override { return this->line.c_str(); }

    private:
      const command::Request &request;
      const loc::Localization &localizator;
      int code;
      std::optional<std::string> message;
      std::string line;
      ResponseError error;
  };

  template <ResponseError T>
  class ResponseException<
      T, typename std::enable_if<T == NOT_ENOUGH_ARGUMENTS>::type>
      : public std::exception {
    public:
      ResponseException(const command::Request &request,
                        const loc::Localization &localizator,
                        command::CommandArgument argument)
          : request(request),
            localizator(localizator),
            argument(argument),
            error(T) {
        loc::LineId line_id = loc::LineId::ErrorNotEnoughArguments;
        loc::LineId arg_id;

        switch (this->argument) {
          case command::SUBCOMMAND:
            arg_id = loc::LineId::ArgumentSubcommand;
            break;
          case command::MESSAGE:
            arg_id = loc::LineId::ArgumentMessage;
            break;
          case command::NAME:
            arg_id = loc::LineId::ArgumentName;
            break;
          case command::TARGET:
            arg_id = loc::LineId::ArgumentTarget;
            break;
          case command::VALUE:
            arg_id = loc::LineId::ArgumentValue;
            break;
          default:
            break;
        }

        auto arg =
            this->localizator
                .get_localized_line(
                    this->request.requester.channel_preferences.get_locale(),
                    arg_id)
                .value();

        this->line =
            this->localizator.get_formatted_line(this->request, line_id, {arg})
                .value();
      }
      ~ResponseException() = default;

      const char *what() const noexcept override { return this->line.c_str(); }

    private:
      const command::Request &request;
      const loc::Localization &localizator;
      command::CommandArgument argument;
      ResponseError error;
      std::string line;
  };
}
