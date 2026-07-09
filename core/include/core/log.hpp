#pragma once

#include <string>

namespace bot {
  enum class LogLevel { INFO = 0, DEBUG = 1, WARN = 2, ERROR = 3 };

  class Logger {
    public:
      Logger() : name("dnb") {}
      Logger(const std::string &name) : name(name) {}

      void info(const std::string &message) const;
      void debug(const std::string &message) const;
      void warn(const std::string &message) const;
      void error(const std::string &message) const;

    private:
      std::string name;

      void log(const LogLevel &level, const std::string &message) const;
  };
}