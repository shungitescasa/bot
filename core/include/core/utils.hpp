#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace bot::utils {
  namespace string {
    void replace(std::string &str, const std::string &from,
                 const std::string &to);

    std::vector<std::string> split_and_collect(std::string input,
                                               char delimiter);

    std::string join(std::vector<std::string> v, std::string delimiter);
  }

  namespace chrono {
    std::chrono::system_clock::time_point string_to_time_point(
        const std::string &value,
        const std::string &format = "%Y-%m-%d %H:%M:%S");

    std::string humanize_timestamp(long long seconds);
  }
}
