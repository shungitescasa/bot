#include "core/utils.hpp"

#include <iomanip>

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