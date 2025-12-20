#include "emotes.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "database.hpp"
#include "logger.hpp"
#include "schemas/stream.hpp"
#include "utils/string.hpp"

namespace bot::emotes {
  void handle_emote_event(const EmoteEventBundle &bundle,
                          const schemas::EventType &event_type,
                          const std::string &channel_name,
                          const std::optional<std::string> &author_id,
                          const emotespp::Emote &emote) {
    std::string c_name = "", author_name = "-", prefix = "";

    if (author_id.has_value()) {
      std::optional<emotespp::User> user =
          bundle.stv_api_client.get_user(author_id.value());

      if (user.has_value()) {
        author_name = user->username;
      }
    }

    bool is_stv = event_type >= schemas::EventType::STV_EMOTE_CREATE &&
                  event_type <= schemas::EventType::STV_EMOTE_UPDATE;

#ifdef BUILD_BETTERTTV
    bool is_bttv = event_type >= schemas::EventType::BTTV_EMOTE_CREATE &&
                   event_type <= schemas::EventType::BTTV_EMOTE_UPDATE;
#endif

    if (is_stv) {
      std::optional<emotespp::EmoteSet> emote_set =
          bundle.stv_api_client.get_emote_set(channel_name);
      if (!emote_set.has_value()) {
        return;
      }

      c_name = emote_set->owner.alias_id;
      prefix = "(7TV)";
    }
#ifdef BUILD_BETTERTTV
    else if (is_bttv) {
      c_name = channel_name.substr(7);
      prefix = "(BTTV)";
    }
#endif

    if (c_name.empty()) {
      return;
    }

    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(bundle.configuration);

    db::DatabaseRows events = conn->exec(
        "SELECT e.id, e.message, is_massping, c.alias_name AS channel_aname, "
        "c.alias_id AS channel_aid FROM "
        "events e "
        "INNER JOIN channels c ON c.id = e.channel_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        {std::to_string(static_cast<int>(event_type)), c_name});

    for (const db::DatabaseRow &event : events) {
      std::vector<std::string> names;

      bool massping_enabled = std::stoi(event.at("is_massping"));

      if (massping_enabled) {
        auto chatters =
            bundle.helix_client.get_chatters(std::stoi(event.at("channel_aid")),
                                             bundle.irc_client.get_user_id());

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

      std::string base = prefix + " " + event.at("message");
      if (!names.empty()) {
        base.append(" · ");
      }

      int pos = base.find("{author}");
      if (pos != std::string::npos) {
        base.replace(pos, 8, author_name);
      }

      pos = base.find("{emote}");
      if (pos != std::string::npos) {
        base.replace(pos, 7, emote.code);
      }

      pos = base.find("{old_emote}");
      if (pos != std::string::npos) {
        base.replace(pos, 11, emote.original_code.value_or("-"));
      }

      std::vector<std::string> msgs =
          utils::string::separate_by_length(base, names, "@", " ", 500);

      for (const auto &msg : msgs) {
        bundle.irc_client.say(event.at("channel_aname"), base + msg);
      }
    }
  }

  void check_seventv_emotesets(const EmoteEventBundle *bundle,
                               std::unique_ptr<db::BaseDatabase> &conn) {
    db::DatabaseRows events = conn->exec(
        "SELECT name FROM events WHERE event_type >= 10 AND event_type <= "
        "12 GROUP BY name");

    auto &ids = bundle->stv_ws_client.get_ids();

    std::vector<std::string> names;
    std::for_each(
        events.begin(), events.end(),
        [&names](const db::DatabaseRow &r) { names.push_back(r.at("name")); });

    // adding new emote sets
    for (const std::string &name : names) {
      std::optional<emotespp::User> stv_user =
          bundle->stv_api_client.get_user_by_twitch_id(std::stoi(name));

      if (!stv_user.has_value()) {
        continue;
      }

      if (!std::any_of(ids.begin(), ids.end(),
                       [&stv_user](const std::string &id) {
                         return id == stv_user->emote_set_id;
                       })) {
        bundle->stv_ws_client.subscribe_emote_set(stv_user->emote_set_id);
        log::info(
            "emotes/thread/7tv",
            "Subscribing to " + stv_user->emote_set_id + " (" + name + ")");
      }
    }

    // removing old emote sets
    std::for_each(
        ids.begin(), ids.end(), [&names, &bundle](const std::string &id) {
          std::optional<emotespp::EmoteSet> stv_set =
              bundle->stv_api_client.get_emote_set(id);

          if (!stv_set.has_value()) {
            return;
          }

          if (!std::any_of(names.begin(), names.end(),
                           [&stv_set](const std::string &id) {
                             return id == stv_set->owner.alias_id;
                           })) {
            bundle->stv_ws_client.unsubscribe_emote_set(id);
            log::info("emotes/thread/7tv", "Unsubscribing from " + id + " (" +
                                               stv_set->owner.alias_id + ")");
          }
        });
  }

#ifdef BUILD_BETTERTTV
  void check_betterttv_users(const EmoteEventBundle *bundle,
                             std::unique_ptr<db::BaseDatabase> &conn) {
    db::DatabaseRows events = conn->exec(
        "SELECT name FROM events WHERE event_type >= 13 AND event_type <= "
        "15 GROUP BY name");

    const std::map<std::string, std::vector<emotespp::Emote>> &ids =
        bundle->bttv_ws_client.get_ids();

    std::vector<std::string> names;
    std::for_each(events.begin(), events.end(),
                  [&names](const db::DatabaseRow &r) {
                    names.push_back("twitch:" + r.at("name"));
                  });

    // adding new users
    for (const std::string &name : names) {
      if (!std::any_of(ids.begin(), ids.end(), [&name](const auto &pair) {
            return pair.first == name;
          })) {
        bundle->bttv_ws_client.subscribe_emote_set(name);
        log::info("emotes/thread/bttv", "Subscribing to " + name);
      }
    }

    // removing old users
    std::for_each(ids.begin(), ids.end(), [&names, &bundle](const auto &pair) {
      if (!std::any_of(
              names.begin(), names.end(),
              [&pair](const std::string &id) { return id == pair.first; })) {
        bundle->bttv_ws_client.unsubscribe_emote_set(pair.first);
        log::info("emotes/thread/bttv", "Unsubscribing from " + pair.first);
      }
    });
  }
#endif

  void create_emote_thread(const EmoteEventBundle *bundle) {
    log::info("emotes/thread", "Started emote thread.");

    while (true) {
      std::unique_ptr<db::BaseDatabase> conn =
          db::create_connection(bundle->configuration);

      try {
        check_seventv_emotesets(bundle, conn);
#ifdef BUILD_BETTERTTV
        check_betterttv_users(bundle, conn);
#endif
      } catch (std::exception ex) {
        log::error("emotes/thread",
                   "Error occurred in emote thread: " + std::string(ex.what()));
      }

      conn->close();

      std::this_thread::sleep_for(std::chrono::seconds(30));
    }
  }
}