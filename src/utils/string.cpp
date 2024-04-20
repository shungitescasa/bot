#include "string.hpp"

#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> split_text(const std::string &text, char delimiter) {
  std::vector<std::string> parts;

  std::istringstream iss(text);
  std::string part;

  while (std::getline(iss, part, delimiter)) {
    parts.push_back(part);
  }

  return parts;
}
