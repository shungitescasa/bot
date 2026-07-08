#include "core/utils.hpp"

#include <iomanip>
#include <ranges>
#include <string>
#include <vector>

namespace bot::utils {

  namespace string {
    void replace(std::string &str, const std::string &from,
                 const std::string &to) {
      if (from.empty()) return;
      int pos = 0;
      while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
      }
    }

    std::vector<std::string> split_and_collect(std::string input,
                                               char delimiter) {
      std::vector<std::string> parts;
      for (auto sv : std::ranges::views::split(input, delimiter)) {
        parts.emplace_back(std::string(sv.begin(), sv.end()));
      }
      return parts;
    }

    std::string join(std::vector<std::string> v, std::string delimiter) {
      std::string o;
      for (const auto &part :
           std::views::all(v) | std::views::join_with(delimiter))
        o += part;
      return o;
    }
  }

  namespace chrono {
    std::chrono::system_clock::time_point string_to_time_point(
        const std::string &value, const std::string &format) {
      std::tm tm = {};
      std::stringstream ss(value);

      ss >> std::get_time(&tm, format.c_str());

      if (ss.fail()) {
        throw std::invalid_argument("Invalid time format");
      }

      return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
  }
}