#include "utils/events.hpp"

#include <memory>
#include <string>
#include <vector>

namespace bot::utils {
  std::vector<schemas::Event> get_events(std::unique_ptr<db::BaseDatabase> conn,
                                         api::twitch::HelixClient &api_client,
                                         int moderator_id, int type,
                                         const std::string &name) {
    std::vector<schemas::Event> events;

    db::DatabaseRows rows = conn->exec(
        "SELECT e.id, e.message, is_massping, c.alias_name AS channel_aname, "
        "c.alias_id AS channel_aid FROM "
        "events e "
        "INNER JOIN channels c ON c.id = e.channel_id "
        "WHERE e.event_type = $1 AND e.name = $2",
        {std::to_string(type), name});

    for (const db::DatabaseRow &row : rows) {
      schemas::Event event;

      event.id = std::stoi(row.at("id"));
      event.alias_id = std::stoi(row.at("channel_aid"));
      event.is_massping = std::stoi(row.at("is_massping"));
      event.message = row.at("message");
      event.channel_alias_name = row.at("channel_aname");

      if (event.is_massping) {
        auto chatters = api_client.get_chatters(event.alias_id, moderator_id);

        std::for_each(
            chatters.begin(), chatters.end(),
            [&event](const auto &x) { event.subs.push_back(x.login); });
      } else {
        db::DatabaseRows subs = conn->exec(
            "SELECT u.alias_name FROM users u "
            "INNER JOIN events e ON e.id = $1 "
            "INNER JOIN event_subscriptions es ON es.event_id = e.id "
            "WHERE u.id = es.user_id",
            {std::to_string(event.id)});

        std::for_each(subs.begin(), subs.end(),
                      [&event](const db::DatabaseRow &x) {
                        event.subs.push_back(x.at("alias_name"));
                      });
      }

      events.push_back(event);
    }

    return events;
  }
}