#include "chrono.hpp"

#include <cmath>
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
}
