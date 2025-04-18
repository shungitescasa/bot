#include "emotes.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <thread>
#include <vector>

#include "config.hpp"
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
                  schemas::EventType::STV_EMOTE_UPDATE >= event_type;

    if (is_stv) {
      std::optional<emotespp::EmoteSet> emote_set =
          bundle.stv_api_client.get_emote_set(channel_name);
      if (!emote_set.has_value()) {
        return;
      }

      c_name = emote_set->owner.alias_id;
      prefix = "(7TV)";
    }

    if (c_name.empty()) {
      return;
    }

    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(bundle.configuration));
    pqxx::work work(conn);

    pqxx::result events = work.exec_params(
        "SELECT e.id, e.message, array_to_json(e.flags) AS "
        "flags, c.alias_name AS channel_aname, c.alias_id AS channel_aid FROM "
        "events e "
        "INNER JOIN channels c ON c.id = e.channel_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        pqxx::params{static_cast<int>(event_type), c_name});

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
        auto chatters = bundle.helix_client.get_chatters(
            event[4].as<int>(), bundle.irc_client.get_bot_id());

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

      std::string base = prefix + " " + event[1].as<std::string>();
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
        bundle.irc_client.say(event[3].as<std::string>(), base + msg);
      }
    }

    work.commit();
    conn.close();
  }

  void create_emote_thread(const EmoteEventBundle *bundle) {
    log::info("emotes/thread", "Started emote thread.");

    while (true) {
      pqxx::connection conn(GET_DATABASE_CONNECTION_URL(bundle->configuration));
      pqxx::work work(conn);

      try {
        pqxx::result events = work.exec(
            "SELECT name FROM events WHERE event_type >= 10 AND event_type <= "
            "12 GROUP BY name");

        auto &ids = bundle->stv_ws_client.get_ids();

        std::vector<std::string> names;
        std::for_each(events.begin(), events.end(),
                      [&names](const pqxx::row &r) {
                        names.push_back(r[0].as<std::string>());
                      });

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
                "emotes/thread",
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
                log::info("emotes/thread", "Unsubscribing from " + id + " (" +
                                               stv_set->owner.alias_id + ")");
              }
            });
      } catch (std::exception ex) {
        log::error("emotes/thread",
                   "Error occurred in emote thread: " + std::string(ex.what()));
      }

      work.commit();
      conn.close();

      std::this_thread::sleep_for(std::chrono::seconds(30));
    }
  }
}