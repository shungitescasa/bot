#pragma once

#include <string>
#include <vector>

namespace bot {
  namespace utils {
    namespace string {
      std::vector<std::string> split_text(const std::string &text,
                                          char delimiter);
      std::string join_vector(const std::vector<std::string> &vec,
                              char delimiter);
      std::string join_vector(const std::vector<std::string> &vec);
    }
  }
}
