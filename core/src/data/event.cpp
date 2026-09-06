#include "core/data/event.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core/data/database.hpp"
#include "core/externalapi/twitch.hpp"

namespace bot::data {
  Event::Event(const DatabaseRow &row) {
    this->id = std::stoi(row.at("id"));
    this->is_massping = std::stoi(row.at("is_massping"));
    this->message = row.at("message");
    this->room_name = row.at("room_name");
    this->room_alias_id = std::stoi(row.at("room_alias_id"));
  }

  std::string Event::create_event_message(const std::string &type,
                                          const std::string &name,
                                          const RSSItem &item) {
    std::string msg = this->message, t = item.title;

    std::stringstream ss;

    // Twitch Livestreams
    if (type.starts_with("twitch.") || type.starts_with("kick.")) {
      ss << "⚡";
      int new_value = msg.find("{new}");

      if (new_value != std::string::npos) {
        if (type == "twitch.title" || type == "kick.title") {
          msg.replace(new_value, 5,
                      t.substr(std::string("title changed to: ").size()));
        } else if (type == "twitch.game" || type == "kick.game") {
          if (t == "stopped playing a game") {
            msg.replace(new_value, 5, "*nothing*");
          } else {
            msg.replace(new_value, 5,
                        t.substr(std::string("now playing: ").size()));
          }
        }
      }
    }
    // 7TV & BetterTTV
    else if (type.starts_with("7tv.") || type.starts_with("bttv.")) {
      if (type.starts_with("7tv.")) {
        ss << "(7TV)";
      } else if (type.starts_with("bttv.")) {
        ss << "(BTTV)";
      }

      int new_value = msg.find("{new}");
      if (new_value != std::string::npos) {
        std::string s = t;

        if (t.starts_with("added a new emote: ") ||
            t.starts_with("removed emote")) {
          int st = s.find(": ") + 2;
          s = s.substr(st, s.find(" (") - st);
        } else if (t.starts_with("renamed emote: ")) {
          s = s.substr(s.find(" to ") + 4);
        }

        msg.replace(new_value, 5, s);
      }

      int old_value = msg.find("{old}");
      if (old_value != std::string::npos) {
        if (t.contains(" (")) {
          int st = t.find(" (") + 2;
          msg.replace(old_value, 5, t.substr(st, t.size() - st - 1));
        } else if (t.contains(" to ")) {
          int st = t.find(": ") + 2;
          msg.replace(old_value, 5, t.substr(st, t.find(" to ") - st));
        }
      }
    }
    // GitHub
    else if (type.starts_with("github.")) {
      ss << "👨🏻‍💻";

      int sha_value = msg.find("{sha}");
      if (sha_value != std::string::npos) {
        msg.replace(sha_value, 5, t.substr(t.size() - 9, t.size() - 1));
      }

      int message_value = msg.find("{message}");
      if (message_value != std::string::npos) {
        msg.replace(message_value, 9, t.substr(t.find(": "), t.size() - 10));
      }
    }
    // Other cases
    else {
      if (type.starts_with("telegram.")) {
        ss << "⌲";
      } else if (type.starts_with("twitter.")) {
        ss << "𝕏";
      } else {
        ss << "🛜";
      }

      int msg_value = msg.find("{message}");
      if (msg_value != std::string::npos) msg.replace(msg_value, 9, item.title);
    }

    // setting a link
    int link_value = msg.find("{link}");
    if (link_value != std::string::npos && !item.link.empty()) {
      msg.replace(link_value, 6, item.link);
    }

    // setting an author
    int author_value = msg.find("{author}");
    if (author_value != std::string::npos && !item.author.empty()) {
      msg.replace(author_value, 8, item.author);
    }

    ss << " ";
    ss << msg;
    if (!this->subs.empty()) ss << " · ";

    return ss.str();
  }

  std::vector<Event> get_events(const std::string &type,
                                const std::string &name) {
    std::vector<Event> events;

    DatabaseConnection conn = create_connection();
    DatabaseRows rows = conn->exec(
        "SELECT e.id, e.message, e.is_massping, r.name AS room_name, "
        "r.alias_id AS room_alias_id FROM "
        "events e "
        "INNER JOIN rooms r ON r.id = e.room_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        {type, name});

    for (const DatabaseRow &row : rows) {
      Event event(row);

      if (event.is_massping) {
        auto &api = externalapi::twitch::HelixClient::get_instance();
        auto chatters = api.get_chatters(std::stoi(name));
        std::for_each(
            chatters.begin(), chatters.end(),
            [&event](const auto &x) { event.subs.push_back(x.login); });
      } else {
        DatabaseRows subs = conn->exec(
            "SELECT s.name FROM senders s "
            "INNER JOIN events e ON e.id = $1 "
            "INNER JOIN event_subscriptions es ON es.event_id = e.id "
            "WHERE s.id = es.sender_id",
            {std::to_string(event.id)});

        std::for_each(subs.begin(), subs.end(), [&event](const DatabaseRow &x) {
          event.subs.push_back(x.at("name"));
        });
      }

      events.push_back(event);
    }

    return events;
  }
}
