#pragma once

#include <string>

namespace bot::log {
  enum LogLevel { INFO, DEBUG, WARN, ERROR };

  void log(const LogLevel &level, const std::string &source,
           const std::string &message);

  // just shorthands
  void info(const std::string &source, const std::string &message);
  void debug(const std::string &source, const std::string &message);
  void warn(const std::string &source, const std::string &message);
  void error(const std::string &source, const std::string &message);
}
