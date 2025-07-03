#include "logger.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace bot::log {
  void log(const LogLevel &level, const std::string &source,
           const std::string &message) {
    std::string dir_name = "logs";
    if (!std::filesystem::exists(dir_name)) {
      std::filesystem::create_directory(dir_name);
    }

    if (std::filesystem::exists(dir_name) &&
        !std::filesystem::is_directory(dir_name)) {
      throw std::runtime_error("The path '" + dir_name +
                               "' is not a directory!");
      return;
    }

    std::ostringstream line;

    // getting time
    std::time_t current_time = std::time(nullptr);
    std::tm *local_time = std::localtime(&current_time);

    line << "[" << std::put_time(local_time, "%H:%M:%S") << " ";

    std::string level_str;

    switch (level) {
      case DEBUG:
        level_str = "\x1B[42mDEBUG\033[0m";
        break;
      case WARN:
        level_str = "\x1B[43mWARN\033[0m";
        break;
      case ERROR:
        level_str = "\x1B[41mERROR\033[0m";
        break;
      default:
        level_str = "\x1B[44mINFO\033[0m";
        break;
    }

    line << level_str << " ";

    line << source << "] " << message << "\n";

#ifdef DEBUG_MODE
    std::cout << line.str();
#else
    if (level != LogLevel::DEBUG) {
      std::cout << line.str();
    }
#endif

    // saving into the log file
    std::ostringstream file_name_oss;
    file_name_oss << dir_name << "/";
    file_name_oss << "log_";
    file_name_oss << std::put_time(local_time, "%Y-%m-%d");
    file_name_oss << ".log";

    std::ofstream ofs;
    ofs.open(file_name_oss.str(), std::ios::app);

    if (ofs.is_open()) {
      ofs << line.str();
      ofs.close();
    } else {
      std::cerr << "Failed to write to the log file!\n";
    }
  }

  void info(const std::string &source, const std::string &message) {
    log(LogLevel::INFO, source, message);
  }

  void debug(const std::string &source, const std::string &message) {
    log(LogLevel::DEBUG, source, message);
  }

  void warn(const std::string &source, const std::string &message) {
    log(LogLevel::WARN, source, message);
  }

  void error(const std::string &source, const std::string &message) {
    log(LogLevel::ERROR, source, message);
  }
}
