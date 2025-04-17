#include "stream.hpp"

#include <algorithm>
#include <chrono>
#include <pqxx/pqxx>
#include <string>
#include <thread>
#include <vector>

#include "api/twitch/schemas/stream.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "schemas/stream.hpp"
#include "utils/string.hpp"

namespace bot::stream {
  void StreamListenerClient::listen_channel(const int &id) {
    this->streamers.push_back({id, TWITCH, false, "", ""});
    log::info("TwitchStreamListener",
              "Listening stream events for ID " + std::to_string(id));
  }

  void StreamListenerClient::unlisten_channel(const int &id) {
    auto x = std::find_if(this->streamers.begin(), this->streamers.end(),
                          [&id](const StreamerData &x) { return x.id == id; });

    if (x != this->streamers.end()) {
      this->streamers.erase(x);
    }
  }

  void StreamListenerClient::run() {
    while (true) {
      this->update_channel_ids();
      this->check();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }

  void StreamListenerClient::check() {
    std::vector<int> twitch_ids;

    std::for_each(this->streamers.begin(), this->streamers.end(),
                  [&twitch_ids](const StreamerData &data) {
                    if (data.type == TWITCH) {
                      twitch_ids.push_back(data.id);
                    }
                  });

    auto twitch_streams = this->helix_client.get_streams(twitch_ids);
    auto now = std::chrono::system_clock::now();
    auto now_time_it = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::gmtime(&now_time_it);
    now = std::chrono::system_clock::from_time_t(std::mktime(now_tm));

    // notifying about new livestreams
    for (const auto &stream : twitch_streams) {
      auto data = std::find_if(this->streamers.begin(), this->streamers.end(),
                               [&stream](const StreamerData &data) {
                                 return data.type == TWITCH &&
                                        data.id == stream.get_user_id();
                               });

      if (data == this->streamers.end()) {
        continue;
      }

      if (!data->is_live) {
        data->is_live = true;

        auto difference = now - stream.get_started_at();
        auto difference_min =
            std::chrono::duration_cast<std::chrono::minutes>(difference);

        if (difference_min.count() < 1) {
          this->handler(schemas::EventType::LIVE, stream, *data);
        }
      }
    }

    // removing ended livestreams
    for (StreamerData &data : this->streamers) {
      if (data.type != TWITCH) {
        continue;
      }

      if (data.is_live &&
          !std::any_of(
              twitch_streams.begin(), twitch_streams.end(),
              [&data](const auto &s) { return s.get_user_id() == data.id; })) {
        data.is_live = false;
        this->handler(schemas::EventType::OFFLINE,
                      api::twitch::schemas::Stream{data.id}, data);
      }
    }

    // notifying about stream info
    auto twitch_stream_info =
        this->helix_client.get_channel_information(twitch_ids);

    for (const auto &stream : twitch_stream_info) {
      auto data = std::find_if(this->streamers.begin(), this->streamers.end(),
                               [&stream](const StreamerData &data) {
                                 return data.type == TWITCH &&
                                        data.id == stream.get_user_id();
                               });

      if (data == this->streamers.end()) {
        continue;
      }

      if (stream.get_title() != data->title) {
        if (!data->title.empty()) {
          this->handler(schemas::EventType::TITLE, stream, *data);
        }

        data->title = stream.get_title();
      }

      if (stream.get_game_name() != data->game) {
        if (!data->game.empty()) {
          this->handler(schemas::EventType::GAME, stream, *data);
        }

        data->game = stream.get_game_name();
      }
    }
  }

  void StreamListenerClient::handler(const schemas::EventType &type,
                                     const api::twitch::schemas::Stream &stream,
                                     const StreamerData &data) {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result events = work.exec_params(
        "SELECT e.id, e.message, array_to_json(e.flags) AS "
        "flags, c.alias_name AS channel_aname, c.alias_id AS channel_aid FROM "
        "events e "
        "INNER JOIN channels c ON c.id = e.channel_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        pqxx::params{static_cast<int>(type), stream.get_user_id()});

    for (const auto &event : events) {
      std::vector<std::string> names;

      bool massping_enabled = false;
      if (!event[2].is_null()) {
        nlohmann::json j = nlohmann::json::parse(event[2].as<std::string>());
        massping_enabled = std::any_of(j.begin(), j.end(), [](const auto &x) {
          return static_cast<int>(x) == static_cast<int>(schemas::MASSPING);
        });
      }

      if (massping_enabled) {
        auto chatters = this->helix_client.get_chatters(
            event[4].as<int>(), this->irc_client.get_bot_id());

        std::for_each(chatters.begin(), chatters.end(),
                      [&names](const auto &x) { names.push_back(x.login); });
      } else {
        pqxx::result subs = work.exec_params(
            "SELECT u.alias_name FROM users u "
            "INNER JOIN events e ON e.id = $1 "
            "INNER JOIN event_subscriptions es ON es.event_id = e.id "
            "WHERE u.id = es.user_id",
            pqxx::params{event[0].as<int>()});

        std::for_each(subs.begin(), subs.end(), [&names](const pqxx::row &x) {
          names.push_back(x[0].as<std::string>());
        });
      }

      std::string base = "⚡ " + event[1].as<std::string>();
      if (!names.empty()) {
        base.append(" · ");
      }

      int pos = base.find("{old}");
      if (pos != std::string::npos) {
        if (type == schemas::EventType::TITLE)
          base.replace(pos, 5, data.title);
        else if (type == schemas::EventType::GAME)
          base.replace(pos, 5, data.game);
      }

      pos = base.find("{new}");
      if (pos != std::string::npos) {
        if (type == schemas::EventType::TITLE)
          base.replace(pos, 5, stream.get_title());
        else if (type == schemas::EventType::GAME)
          base.replace(pos, 5, stream.get_game_name());
      }

      std::vector<std::string> msgs =
          utils::string::separate_by_length(base, names, "@", " ", 500);

      for (const auto &msg : msgs) {
        this->irc_client.say(event[3].as<std::string>(), base + msg);
      }
    }

    work.commit();
    conn.close();
  }

  void StreamListenerClient::update_channel_ids() {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result ids =
        work.exec("SELECT name FROM events WHERE event_type < 10");

    for (const auto &row : ids) {
      int id = row[0].as<int>();

      if (std::any_of(this->streamers.begin(), this->streamers.end(),
                      [&id](const StreamerData &x) {
                        return x.type == TWITCH && x.id == id;
                      })) {
        continue;
      }

      listen_channel(id);
    }

    work.commit();
    conn.close();
  }
}
