#pragma once

#include <chrono>
#include <string>

namespace bot::utils::chrono {
  std::string format_timestamp(int seconds);
  std::chrono::system_clock::time_point string_to_time_point(
      const std::string &value,
      const std::string &format = "%Y-%m-%d %H:%M:%S");
}
