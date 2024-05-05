#pragma once

#include <optional>
#include <string>
#include <type_traits>

#include "command.hpp"

namespace bot::command {
  enum ResponseErrorType {
    NOT_ENOUGH_ARGUMENTS,
    INCORRECT_ARGUMENT,

    INCOMPATIBLE_NAME,
    NAMESAKE_CREATION,
    NOT_FOUND,

    SOMETHING_WENT_WRONG,

    EXTERNAL_API_ERROR,
    INSUFFICIENT_RIGHTS
  };

  template <ResponseErrorType T, class Enable = void>
  class ResponseError;

  template <ResponseErrorType T>
  class ResponseError<T,
                      typename std::enable_if<
                          T == INCORRECT_ARGUMENT || T == INCOMPATIBLE_NAME ||
                          T == NAMESAKE_CREATION || T == NOT_FOUND>::type> {
    public:
      ResponseError(const std::string &message) : message(message), m_type(T){};
      ~ResponseError() = default;

      const std::string &what() const noexcept { return this->message; }

      const ResponseErrorType &type() const noexcept { return this->m_type; }

    private:
      std::string message;
      ResponseErrorType m_type;
  };

  template <ResponseErrorType T>
  class ResponseError<T,
                      typename std::enable_if<T == SOMETHING_WENT_WRONG ||
                                              T == INSUFFICIENT_RIGHTS>::type> {
    public:
      ResponseError() : m_type(T){};
      ~ResponseError() = default;

      const ResponseErrorType &type() const noexcept { return this->m_type; }

    private:
      ResponseErrorType m_type;
  };

  template <ResponseErrorType T>
  class ResponseError<T,
                      typename std::enable_if<T == EXTERNAL_API_ERROR>::type> {
    public:
      ResponseError(const int &code, const std::optional<std::string> &message)
          : m_code(code), message(message), m_type(T){};
      ~ResponseError() = default;

      const std::optional<std::string> &what() const noexcept {
        return this->message;
      }
      const int &code() const noexcept { return this->code; }
      const ResponseErrorType &type() const noexcept { return this->m_type; }

    private:
      int m_code;
      std::optional<std::string> message;
      ResponseErrorType m_type;
  };

  template <ResponseErrorType T>
  class ResponseError<
      T, typename std::enable_if<T == NOT_ENOUGH_ARGUMENTS>::type> {
    public:
      ResponseError(const CommandArgument &argument)
          : m_argument(argument), m_type(T){};
      ~ResponseError() = default;

      const CommandArgument &argument() const noexcept {
        return this->m_argument;
      }
      const ResponseErrorType &type() const noexcept { return this->m_type; }

    private:
      CommandArgument m_argument;
      ResponseErrorType m_type;
  };
}
