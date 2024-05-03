#pragma once

#include <string>
namespace bot::api::twitch::schemas {
  struct Stream {
      int user_id;
      std::string user_login, game_name, title, started_at;
  };
}
