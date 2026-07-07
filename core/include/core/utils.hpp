#pragma once

#include <chrono>
#include <string>

namespace bot::utils {
  namespace string {
    void replace(std::string &str, const std::string &from,
                 const std::string &to);
  }

  namespace chrono {
    std::chrono::system_clock::time_point string_to_time_point(
        const std::string &value,
        const std::string &format = "%Y-%m-%d %H:%M:%S");
  }
}