#include "stream.hpp"

#include <algorithm>
#include <chrono>
#include <pqxx/pqxx>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "api/twitch/schemas/stream.hpp"
#include "config.hpp"
#include "logger.hpp"
#include "schemas/stream.hpp"
#include "utils/string.hpp"

namespace bot::stream {
  void StreamListenerClient::listen_channel(const int &id) {
    this->ids.push_back(id);
  }
  void StreamListenerClient::unlisten_channel(const int &id) {
    auto x = std::find_if(this->ids.begin(), this->ids.end(),
                          [&](const auto &x) { return x == id; });

    if (x != this->ids.end()) {
      this->ids.erase(x);
    }

    auto y = std::find_if(this->online_ids.begin(), this->online_ids.end(),
                          [&](const auto &x) { return x == id; });

    if (y != this->online_ids.end()) {
      this->online_ids.erase(y);
    }
  }
  void StreamListenerClient::run_thread() {
    std::thread t(&bot::stream::StreamListenerClient::run, this);
    t.join();
  }
  void StreamListenerClient::run() {
    while (true) {
      this->update_channel_ids();
      this->check();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
  void StreamListenerClient::check() {
    auto streams = this->helix_client.get_streams(this->ids);

    // adding new ids
    for (const auto &stream : streams) {
      bool is_already_live =
          std::any_of(this->online_ids.begin(), this->online_ids.end(),
                      [&](const auto &x) { return x == stream.get_user_id(); });

      if (!is_already_live) {
        this->online_ids.insert(stream.get_user_id());
        this->handler(schemas::EventType::LIVE, stream);
      }
    }

    // removing old ids
    for (auto i = this->online_ids.begin(); i != this->online_ids.end();) {
      auto stream =
          std::find_if(streams.begin(), streams.end(),
                       [&](const auto &x) { return x.get_user_id() == *i; });

      if (stream == streams.end()) {
        this->handler(schemas::EventType::OFFLINE,
                      api::twitch::schemas::Stream{*i});
        i = this->online_ids.erase(i);
      } else {
        ++i;
      }
    }
  }
  void StreamListenerClient::handler(
      const schemas::EventType &type,
      const api::twitch::schemas::Stream &stream) {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result events = work.exec(
        "SELECT id, channel_id, message, flags FROM events WHERE event_type "
        "= " +
        std::to_string(type) +
        " AND target_alias_id = " + std::to_string(stream.get_user_id()));

    for (const auto &event : events) {
      pqxx::row channel = work.exec1(
          "SELECT alias_id, alias_name, opted_out_at FROM channels WHERE id "
          "= " +
          std::to_string(event[1].as<int>()));

      if (!channel[2].is_null()) {
        continue;
      }

      pqxx::result subs = work.exec(
          "SELECT user_id FROM event_subscriptions WHERE event_id = " +
          std::to_string(event[0].as<int>()));

      std::set<std::string> user_ids;
      if (!subs.empty()) {
        for (const auto &sub : subs) {
          user_ids.insert(std::to_string(sub[0].as<int>()));
        }

        pqxx::result users = work.exec(
            "SELECT alias_name FROM users WHERE id IN (" +
            utils::string::str(user_ids.begin(), user_ids.end(), ',') + ")");

        user_ids.clear();

        for (const auto &user : users) {
          user_ids.insert(user[0].as<std::string>());
        }
      }

      auto flags = event[3].as_array();
      std::pair<pqxx::array_parser::juncture, std::string> elem;

      do {
        elem = flags.get_next();
        if (elem.first == pqxx::array_parser::juncture::string_value) {
          if (std::stoi(elem.second) == schemas::EventFlag::MASSPING) {
            auto chatters = this->helix_client.get_chatters(
                channel[0].as<int>(), this->irc_client.get_bot_id());

            for (const auto &chatter : chatters) {
              user_ids.insert(chatter.login);
            }
          }
        }
      } while (elem.first != pqxx::array_parser::juncture::done);

      std::string base = "⚡ " + event[2].as<std::string>();
      std::vector<std::string> msgs = {""};
      int index = 0;

      if (!user_ids.empty()) {
        base.append(" · ");
      }

      for (const auto &user_id : user_ids) {
        const std::string &current_msg = msgs.at(index);
        std::string x = "@" + user_id;

        if (base.length() + current_msg.length() + 1 + x.length() >= 500) {
          index += 1;
        }

        if (index > msgs.size() - 1) {
          msgs.push_back(x);
        } else {
          msgs[index] = current_msg + " " + x;
        }
      }

      for (const auto &msg : msgs) {
        this->irc_client.say(channel[1].as<std::string>(), base + msg);
      }
    }

    work.commit();
    conn.close();
  }
  void StreamListenerClient::update_channel_ids() {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result ids =
        work.exec("SELECT target_alias_id FROM events WHERE event_type < 99");

    for (const auto &row : ids) {
      int id = row[0].as<int>();

      if (std::any_of(this->ids.begin(), this->ids.end(),
                      [&](const auto &x) { return x == id; })) {
        continue;
      }

      log::info("TwitchStreamListener",
                "Listening stream events for ID " + std::to_string(id));

      this->ids.push_back(id);
    }

    work.commit();
    conn.close();
  }
}
