#pragma once

#include <string>
#include <vector>

namespace bot::schemas {
  struct Event {
      int id, alias_id;
      std::string message, channel_alias_name;
      bool is_massping;
      std::vector<std::string> subs;
  };
}