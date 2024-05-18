#include "chrono.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace bot::utils::chrono {
  std::string format_timestamp(int seconds) {
    int d = round(seconds / (60 * 60 * 24));
    int h = round(seconds / (60 * 60) % 24);
    int m = round(seconds % (60 * 60) / 60);
    int s = round(seconds % 60);

    // Only seconds:
    if (d == 0 && h == 0 && m == 0) {
      return std::to_string(s) + "s";
    }
    // Minutes and seconds:
    else if (d == 0 && h == 0) {
      return std::to_string(m) + "m" + std::to_string(s) + "s";
    }
    // Hours and minutes:
    else if (d == 0) {
      return std::to_string(h) + "h" + std::to_string(m) + "m";
    }
    // Days and hours:
    else {
      return std::to_string(d) + "d" + std::to_string(h) + "h";
    }
  }

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
