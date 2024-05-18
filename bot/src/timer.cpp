#include "timer.hpp"

#include <chrono>
#include <pqxx/pqxx>
#include <string>
#include <thread>

#include "config.hpp"
#include "irc/client.hpp"
#include "utils/chrono.hpp"

namespace bot {
  void create_timer_thread(irc::Client *irc_client,
                           Configuration *configuration) {
    while (true) {
      pqxx::connection conn(GET_DATABASE_CONNECTION_URL_POINTER(configuration));
      pqxx::work *work = new pqxx::work(conn);

      pqxx::result timers = work->exec(
          "SELECT id, interval_sec, message, channel_id, last_executed_at FROM "
          "timers");

      for (const auto &timer : timers) {
        int id = timer[0].as<int>();
        int interval_sec = timer[1].as<int>();
        std::string message = timer[2].as<std::string>();
        int channel_id = timer[3].as<int>();

        // it could be done in sql query
        std::chrono::system_clock::time_point last_executed_at =
            utils::chrono::string_to_time_point(timer[4].as<std::string>());
        auto now = std::chrono::system_clock::now();
        auto now_time_it = std::chrono::system_clock::to_time_t(now);
        auto now_tm = std::gmtime(&now_time_it);
        now = std::chrono::system_clock::from_time_t(std::mktime(now_tm));

        auto difference = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_executed_at);

        if (difference.count() > interval_sec) {
          pqxx::result channels = work->exec(
              "SELECT alias_name, opted_out_at FROM channels WHERE id = " +
              std::to_string(channel_id));

          if (!channels.empty() && channels[0][1].is_null()) {
            std::string alias_name = channels[0][0].as<std::string>();

            irc_client->say(alias_name, message);
          }

          work->exec(
              "UPDATE timers SET last_executed_at = timezone('utc', now()) "
              "WHERE "
              "id = " +
              std::to_string(id));

          work->commit();

          delete work;
          work = new pqxx::work(conn);
        }
      }

      delete work;
      conn.close();

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
