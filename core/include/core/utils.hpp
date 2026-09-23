#pragma once

#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace bot {
  namespace utils {
    namespace string {
      std::string trim(const std::string &input);

      void replace(std::string &str, const std::string &from,
                   const std::string &to);

      std::vector<std::string> split_and_collect(std::string input,
                                                 char delimiter);

      std::string join(std::vector<std::string> v, std::string delimiter);

      std::vector<std::vector<std::string>> separate_by_length(
          const std::vector<std::string> &vector, const int &max_length);

      std::vector<std::string> separate_by_length(
          const std::string &base, const std::vector<std::string> &values,
          const std::string &prefix, const std::string &separator,
          const long long &max_length);
    }

    namespace chrono {
      std::chrono::system_clock::time_point string_to_time_point(
          const std::string &value,
          const std::string &format = "%Y-%m-%d %H:%M:%S");

      std::string humanize_timestamp(long long seconds);
    }
  }
}
