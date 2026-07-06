#pragma once

#include <string>
namespace bot::utils {
  namespace string {
    void replace(std::string &str, const std::string &from,
                 const std::string &to);
  }
}