#include "stream.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "api/kick.hpp"
#include "api/twitch/schemas/stream.hpp"
#include "database.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "schemas/stream.hpp"
#include "utils/string.hpp"

namespace bot::stream {
  void StreamListenerClient::listen_channel(const int &id,
                                            const StreamerType &type) {
    this->streamers.push_back({id, type, false, "", ""});
    log::info("TwitchStreamListener",
              "Listening stream events for ID " + std::to_string(id));
  }

  void StreamListenerClient::unlisten_channel(const int &id,
                                              const StreamerType &type) {
    auto x = std::find_if(this->streamers.begin(), this->streamers.end(),
                          [&id, &type](const StreamerData &x) {
                            return x.id == id && x.type == type;
                          });

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
    std::vector<int> twitch_ids, kick_ids;

    std::for_each(this->streamers.begin(), this->streamers.end(),
                  [&twitch_ids, &kick_ids](const StreamerData &data) {
                    if (data.type == TWITCH) {
                      twitch_ids.push_back(data.id);
                    } else if (data.type == KICK) {
                      kick_ids.push_back(data.id);
                    }
                  });

    auto kick_streams = this->kick_api_client.get_channels(kick_ids);
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

    for (const api::KickChannel &channel : kick_streams) {
      auto data = std::find_if(this->streamers.begin(), this->streamers.end(),
                               [&channel](const StreamerData &data) {
                                 return data.type == KICK &&
                                        data.id == channel.broadcaster_user_id;
                               });

      if (data == this->streamers.end()) {
        continue;
      }

      if (!data->is_live && channel.is_live) {
        data->is_live = true;

        auto difference = now - channel.start_time;
        auto difference_min =
            std::chrono::duration_cast<std::chrono::minutes>(difference);

        if (difference_min.count() < 1) {
          this->handler(schemas::EventType::KICK_LIVE,
                        {channel.broadcaster_user_id, channel.slug,
                         channel.stream_game_name, channel.stream_title},
                        *data);
        }
      }
    }

    // removing ended livestreams
    for (StreamerData &data : this->streamers) {
      bool in_twitch_streams = std::any_of(
          twitch_streams.begin(), twitch_streams.end(), [&data](const auto &s) {
            return s.get_user_id() == data.id && data.type == TWITCH;
          });

      bool in_kick_streams =
          std::any_of(kick_streams.begin(), kick_streams.end(),
                      [&data](const api::KickChannel &s) {
                        return s.broadcaster_user_id == data.id &&
                               data.type == KICK && s.is_live;
                      });

      if (data.type == TWITCH && data.is_live && !in_twitch_streams) {
        data.is_live = false;
        this->handler(schemas::EventType::OFFLINE,
                      api::twitch::schemas::Stream{data.id}, data);
      } else if (data.type == KICK && data.is_live && !in_kick_streams) {
        data.is_live = false;
        this->handler(schemas::EventType::KICK_OFFLINE,
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

    for (const api::KickChannel &channel : kick_streams) {
      auto data = std::find_if(this->streamers.begin(), this->streamers.end(),
                               [&channel](const StreamerData &data) {
                                 return data.type == KICK &&
                                        data.id == channel.broadcaster_user_id;
                               });

      if (data == this->streamers.end()) {
        continue;
      }

      api::twitch::schemas::Stream stream{
          channel.broadcaster_user_id, channel.slug, channel.stream_game_name,
          channel.stream_title};

      if (channel.stream_title != data->title &&
          !channel.stream_title.empty()) {
        if (!data->title.empty()) {
          this->handler(schemas::EventType::KICK_TITLE, stream, *data);
        }

        data->title = channel.stream_title;
      }

      if (channel.stream_game_name != data->game &&
          !channel.stream_game_name.empty()) {
        if (!data->game.empty()) {
          this->handler(schemas::EventType::KICK_GAME, stream, *data);
        }

        data->game = channel.stream_game_name;
      }
    }
  }

  void StreamListenerClient::handler(const schemas::EventType &type,
                                     const api::twitch::schemas::Stream &stream,
                                     const StreamerData &data) {
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(this->configuration);

    db::DatabaseRows events = conn->exec(
        "SELECT e.id, e.message, is_massping, c.alias_name AS channel_aname, "
        "c.alias_id AS channel_aid FROM "
        "events e "
        "INNER JOIN channels c ON c.id = e.channel_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        {std::to_string(static_cast<int>(type)),
         std::to_string(stream.get_user_id())});

    for (const auto &event : events) {
      std::vector<std::string> names;

      bool massping_enabled = std::stoi(event.at("is_massping"));

      if (massping_enabled) {
        auto chatters = this->helix_client.get_chatters(
            std::stoi(event.at("channel_aid")), this->irc_client.get_user_id());

        std::for_each(chatters.begin(), chatters.end(),
                      [&names](const auto &x) { names.push_back(x.login); });
      } else {
        db::DatabaseRows subs = conn->exec(
            "SELECT u.alias_name FROM users u "
            "INNER JOIN events e ON e.id = $1 "
            "INNER JOIN event_subscriptions es ON es.event_id = e.id "
            "WHERE u.id = es.user_id",
            {event.at("id")});

        std::for_each(subs.begin(), subs.end(),
                      [&names](const db::DatabaseRow &x) {
                        names.push_back(x.at("alias_name"));
                      });
      }

      std::string base = "⚡ " + event.at("message");
      if (!names.empty()) {
        base.append(" · ");
      }

      int pos = base.find("{old}");
      if (pos != std::string::npos) {
        if (type == schemas::EventType::TITLE ||
            type == schemas::EventType::KICK_TITLE)
          base.replace(pos, 5, data.title);
        else if (type == schemas::EventType::GAME ||
                 type == schemas::EventType::KICK_GAME)
          base.replace(pos, 5, data.game);
      }

      pos = base.find("{new}");
      if (pos != std::string::npos) {
        if (type == schemas::EventType::TITLE ||
            type == schemas::EventType::KICK_TITLE)
          base.replace(pos, 5, stream.get_title());
        else if (type == schemas::EventType::GAME ||
                 type == schemas::EventType::KICK_GAME)
          base.replace(pos, 5, stream.get_game_name());
      }

      std::vector<std::string> msgs =
          utils::string::separate_by_length(base, names, "@", " ", 500);

      for (const auto &msg : msgs) {
        this->irc_client.say(event.at("channel_aname"), base + msg);
      }
    }

    conn->close();
  }

  void StreamListenerClient::update_channel_ids() {
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(this->configuration);

    db::DatabaseRows ids =
        conn->exec("SELECT name, event_type FROM events WHERE event_type < 10");

    for (const auto &row : ids) {
      int id = std::stoi(row.at("name"));
      int event_type = std::stoi(row.at("event_type"));

      StreamerType type = (event_type >= schemas::EventType::KICK_LIVE &&
                           event_type <= schemas::EventType::KICK_GAME)
                              ? StreamerType::KICK
                              : StreamerType::TWITCH;

      if (std::any_of(this->streamers.begin(), this->streamers.end(),
                      [&id, &type](const StreamerData &x) {
                        return x.type == type && x.id == id;
                      })) {
        continue;
      }

      listen_channel(id, type);
    }

    conn->close();
  }
}
