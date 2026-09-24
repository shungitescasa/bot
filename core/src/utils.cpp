#include "core/utils.hpp"

#include <cmath>
#include <format>
#include <iomanip>
#include <ranges>
#include <string>
#include <vector>

namespace bot::utils {

  namespace string {
    std::string trim(const std::string &input) {
      std::string result;
      result.reserve(input.size());

      bool lastWasSpace = false;
      for (unsigned char c : input) {
        if (std::isspace(c)) {
          lastWasSpace = true;
        } else {
          if (lastWasSpace && !result.empty()) {
            result += ' ';
          }
          result += c;
          lastWasSpace = false;
        }
      }

      return result;
    }

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

    std::vector<std::vector<std::string>> separate_by_length(
        const std::vector<std::string> &vector, const int &max_length) {
      std::vector<std::vector<std::string>> output;
      std::vector<std::string> active;
      int length = 0;

      for (const std::string &str : vector) {
        length += str.length();

        if (length >= max_length) {
          output.push_back(active);
          active = {str};
        } else {
          active.push_back(str);
        }
      }

      if (!active.empty()) output.push_back(active);

      return output;
    }

    std::vector<std::string> separate_by_length(
        const std::string &base, const std::vector<std::string> &values,
        const std::string &prefix, const std::string &separator,
        const long long &max_length) {
      std::vector<std::string> lines = {""};
      int index = 0;

      std::for_each(values.begin(), values.end(), [&](const std::string &v) {
        const std::string &m = lines.at(index);
        std::string x = prefix + v;

        if (base.length() + m.length() + x.length() + separator.length() >=
            max_length) {
          index += 1;
        }

        if (index > lines.size() - 1) {
          lines.push_back(x);
        } else {
          lines[index] = m + separator + x;
        }
      });

      return lines;
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

    std::string humanize_timestamp(long long seconds) {
      long long y = seconds / (60LL * 60 * 24 * 365);
      long long mo = seconds / (60LL * 60 * 24 * 30) % 12;
      long long d = seconds / (60LL * 60 * 24) % 30;
      long long h = seconds / (60LL * 60) % 24;
      long long m = seconds / 60 % 60;
      long long s = seconds % 60;

      // Only seconds:
      if (y == 0 && mo == 0 && d == 0 && h == 0 && m == 0) {
        return std::format("{}s", s);
      }
      // Minutes and seconds:
      else if (y == 0 && mo == 0 && d == 0 && h == 0) {
        return std::format("{}m{}s", m, s);
      }
      // Hours and minutes:
      else if (y == 0 && mo == 0 && d == 0) {
        return std::format("{}h{}m", h, m);
      }
      // Days and hours:
      else if (y == 0 && mo == 0) {
        return std::format("{}d{}h", d, h);
      }
      // Months and days:
      else if (y == 0) {
        return std::format("{}mo{}d", mo, d);
      }
      // Years and months:
      else {
        return std::format("{}y{}mo", y, mo);
      }
    }
  }
}
