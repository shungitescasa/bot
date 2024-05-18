#pragma once

#include <optional>

#include "config.hpp"
#include "crow/http_response.h"

namespace botweb {
  crow::response get_wiki_page(const std::string &path,
                               const std::optional<Configuration> &cfg);
}
