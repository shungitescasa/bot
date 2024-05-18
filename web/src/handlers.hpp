#pragma once

#include "crow/http_response.h"

namespace botweb {
  crow::response get_wiki_page(const std::string &path);
}
