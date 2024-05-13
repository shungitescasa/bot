#include "string.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bot {
  namespace utils {
    namespace string {
      std::vector<std::string> split_text(const std::string &text,
                                          char delimiter) {
        std::vector<std::string> parts;

        std::istringstream iss(text);
        std::string part;

        while (std::getline(iss, part, delimiter)) {
          parts.push_back(part);
        }

        return parts;
      }

      std::string join_vector(const std::vector<std::string> &vec,
                              char delimiter) {
        if (vec.empty()) {
          return "";
        }

        std::string str;

        for (auto i = vec.begin(); i != vec.end() - 1; i++) {
          str += *i + delimiter;
        }

        str += vec[vec.size() - 1];

        return str;
      }

      std::string join_vector(const std::vector<std::string> &vec) {
        std::string str;

        for (const auto &e : vec) {
          str += e;
        }

        return str;
      }

      bool string_contains_sql_injection(const std::string &input) {
        std::string forbidden_strings[] = {";",   "--",     "'",      "\"",
                                           "/*",  "*/",     "xp_",    "exec",
                                           "sp_", "insert", "select", "delete"};

        for (const auto &str : forbidden_strings) {
          if (input.find(str) != std::string::npos) {
            return true;
          }
        }

        return false;
      }
    }
  }
}
