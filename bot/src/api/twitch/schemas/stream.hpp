#pragma once

#include <chrono>
#include <string>

#include "../../../utils/chrono.hpp"

namespace bot::api::twitch::schemas {
  class Stream {
    public:
      Stream(int user_id, std::string user_login, std::string game_name,
             std::string title, std::string started_at)
          : user_id(user_id),
            user_login(user_login),
            game_name(game_name),
            title(title),
            started_at(utils::chrono::string_to_time_point(
                started_at, "%Y-%m-%dT%H:%M:%SZ")) {}

      Stream(int user_id) : user_id(user_id) {}

      const int &get_user_id() const { return this->user_id; }
      const std::string &get_user_login() const { return this->user_login; }
      const std::string &get_game_name() const { return this->game_name; }
      const std::string &get_title() const { return this->title; }
      const std::chrono::system_clock::time_point &get_started_at() const {
        return this->started_at;
      }

    private:
      int user_id;
      std::string user_login, game_name, title;
      std::chrono::system_clock::time_point started_at;
  };
}
