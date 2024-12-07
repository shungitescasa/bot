#pragma once

#include <optional>
#include <string>
#include <vector>

namespace bot::command {
  class Response {
    public:
      Response();
      Response(std::string single);
      Response(std::vector<std::string> multiple);

      const std::string get_single() const;
      const std::vector<std::string> get_multiple() const;

      const bool is_single() const;
      const bool is_multiple() const;
      const bool is_empty() const;

    private:
      std::optional<std::string> single;
      std::optional<std::vector<std::string>> multiple;
  };
}