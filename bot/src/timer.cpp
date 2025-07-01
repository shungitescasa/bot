#include "timer.hpp"

#include <chrono>
#include <string>
#include <thread>

#include "config.hpp"
#include "database.hpp"
#include "irc/client.hpp"
#include "utils/chrono.hpp"

namespace bot {
  void create_timer_thread(irc::Client *irc_client,
                           Configuration *configuration) {
    while (true) {
      std::unique_ptr<db::BaseDatabase> conn =
          db::create_connection(*configuration);

      db::DatabaseRows timers = conn->exec(
          "SELECT id, interval_sec, message, channel_id, last_executed_at FROM "
          "timers");

      for (const auto &timer : timers) {
        int id = std::stoi(timer.at("id"));
        int interval_sec = std::stoi(timer.at("interval_sec"));
        std::string message = timer.at("message");
        int channel_id = std::stoi(timer.at("channel_id"));

        // it could be done in sql query
        std::chrono::system_clock::time_point last_executed_at =
            utils::chrono::string_to_time_point(timer.at("last_executed_at"));
        auto now = std::chrono::system_clock::now();
        auto now_time_it = std::chrono::system_clock::to_time_t(now);
        auto now_tm = std::gmtime(&now_time_it);
        now = std::chrono::system_clock::from_time_t(std::mktime(now_tm));

        auto difference = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_executed_at);

        if (difference.count() > interval_sec) {
          db::DatabaseRows channels = conn->exec(
              "SELECT alias_name, opted_out_at FROM channels WHERE id = $1",
              {std::to_string(channel_id)});

          if (!channels.empty() && channels[0].at("opted_out_at").empty()) {
            std::string alias_name = channels[0].at("alias_name");

            irc_client->say(alias_name, message);
          }

          conn->exec(
              "UPDATE timers SET last_executed_at = UTC_TIMESTAMP "
              "WHERE "
              "id = $1",
              {std::to_string(id)});
        }
      }

      conn->close();

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
