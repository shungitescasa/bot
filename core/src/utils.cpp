#include "core/utils.hpp"

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
}