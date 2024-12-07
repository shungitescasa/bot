#include "response.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bot::command {
  Response::Response() {
    this->single = std::nullopt;
    this->multiple = std::nullopt;
  }

  Response::Response(std::string single) {
    this->single = single;
    this->multiple = std::nullopt;
  }

  Response::Response(std::vector<std::string> multiple) {
    this->single = std::nullopt;
    this->multiple = multiple;
  }

  const std::string Response::get_single() const {
    return this->single.value();
  }

  const std::vector<std::string> Response::get_multiple() const {
    return this->multiple.value();
  }

  const bool Response::is_single() const { return this->single.has_value(); }

  const bool Response::is_multiple() const {
    return this->multiple.has_value();
  }

  const bool Response::is_empty() const {
    return !this->single.has_value() && !this->multiple.has_value();
  }
}