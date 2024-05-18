#pragma once

#include <string>

namespace bot::api::twitch::schemas {
  struct User {
      int id;
      std::string login;
  };
}
