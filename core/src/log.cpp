#include "core/log.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace bot {
  void Logger::log(const LogLevel &level, const std::string &message) const {
    std::ostringstream oss;

    std::time_t current_time = std::time(nullptr);
    std::tm *local_time = std::localtime(&current_time);
    oss << "[" << std::put_time(local_time, "%H:%M:%S") << " ";

    std::string level_str;

    switch (level) {
      case LogLevel::DEBUG:
        oss << "\x1B[42mDEBUG\033[0m ";
        break;
      case LogLevel::WARN:
        oss << "\x1B[43mWARN\033[0m ";
        break;
      case LogLevel::ERROR:
        oss << "\x1B[41mERROR\033[0m ";
        break;
      default:
        oss << "\x1B[44mINFO\033[0m ";
        break;
    }

    oss << this->name << "] " << message << "\n";
#ifdef DEBUG_MODE
    std::cout << oss.str();
#else
    if (level != LogLevel::DEBUG) std::cout << oss.str();
#endif
  }

  void Logger::info(const std::string &message) const {
    this->log(LogLevel::INFO, message);
  }

  void Logger::debug(const std::string &message) const {
    this->log(LogLevel::DEBUG, message);
  }

  void Logger::warn(const std::string &message) const {
    this->log(LogLevel::WARN, message);
  }

  void Logger::error(const std::string &message) const {
    this->log(LogLevel::ERROR, message);
  }
}