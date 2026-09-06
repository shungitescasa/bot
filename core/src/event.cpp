#include "core/event.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "core/config.hpp"
#include "core/data/database.hpp"
#include "cpr/cpr.h"
#include "pugixml.hpp"

namespace bot {
  bool RSSItem::has_category(const std::string &category) const {
    return std::any_of(this->categories.begin(), this->categories.end(),
                       [&](const std::string &x) { return x == category; });
  }

  bool RSSItem::is_event_valid(RSSEvent *e) const {
    std::string type = e->get_type();

    return
        // Twitch
        (type == "twitch.live" && has_category("twitch") &&
         has_category("live")) ||
        (type == "twitch.offline" && has_category("twitch") &&
         has_category("offline")) ||
        (type == "twitch.title" && has_category("twitch") &&
         has_category("title_change")) ||
        (type == "twitch.game" && has_category("twitch") &&
         has_category("game_change")) ||

        // Kick
        (type == "kick.live" && has_category("kick") && has_category("live")) ||
        (type == "kick.offline" && has_category("kick") &&
         has_category("offline")) ||
        (type == "kick.title" && has_category("kick") &&
         has_category("title_change")) ||
        (type == "kick.game" && has_category("kick") &&
         has_category("game_change")) ||

        // 7TV
        (type == "7tv.new-emote" && has_category("7tv") &&
         has_category("new-emote")) ||
        (type == "7tv.deleted-emote" && has_category("7tv") &&
         has_category("deleted-emote")) ||
        (type == "7tv.updated-emote" && has_category("7tv") &&
         has_category("updated-emote")) ||

        // BetterTTV
        (type == "bttv.new-emote" && has_category("bttv") &&
         has_category("new-emote")) ||
        (type == "bttv.deleted-emote" && has_category("bttv") &&
         has_category("deleted-emote")) ||
        (type == "bttv.updated-emote" && has_category("bttv") &&
         has_category("updated-emote")) ||

        // GitHub
        (type == "github.commit" && has_category("github") &&
         has_category("commit")) ||

        // Twitter
        (type == "twitter.post" && has_category("twitter") &&
         has_category("post")) ||

        // GitHub
        (type == "telegram.post" && has_category("telegram") &&
         has_category("post")) ||

        // RSS
        type == "rss";
  }

  RSSEvent::RSSEvent(const std::string &type, const std::string &name) {
    this->name = name;
    this->type = type;

    auto &cfg = Configuration::get_instance();

    if (!cfg.rss.url.has_value()) {
      throw std::runtime_error("No RSS-bridge URL provided");
    }

    std::stringstream url;

    if (this->type == "rss") {
      url << name;
    } else {
      url << *cfg.rss.url;
      url << "/?action=display&format=Mrss&bridge=";
    }

    if (this->type.starts_with("twitch.")) {
      url << "TwitchLivestreamBridge";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("kick.")) {
      url << "KickLivestreamBridge";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("7tv.")) {
      url << "7TVEmoteBridge";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("bttv.")) {
      url << "BTTVEmoteBridge";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("github.")) {
      url << "GithubCommitBridge";
      url << "&u=" << this->name.substr(0, this->name.find("/"));
      url << "&p=" << this->name.substr(this->name.find("/") + 1);
    } else if (this->type.starts_with("twitter.")) {
      url << "TwitterBridge";
      url << "&context=By+username";
      url << "&u=" << this->name;
    } else if (this->type.starts_with("telegram.")) {
      url << "TelegramBridge";
      url << "&username=%40" << this->name;
    }

    else if (this->type != "rss") {
      throw std::runtime_error("Unsupported event type");
    }

    this->url = url.str();
  }

  const std::string &RSSEvent::get_type() const { return this->type; }

  const std::string &RSSEvent::get_name() const { return this->name; }

  const std::string &RSSEvent::get_url() const { return this->url; }

  std::vector<RSSItem> RSSEvent::set_items(std::vector<RSSItem> items) {
    std::unordered_set<std::string> seen;
    seen.reserve(this->items.size());

    for (const auto &m : this->items) seen.insert(m.id);

    std::vector<RSSItem> new_items;
    int item_count = 0;

    for (RSSItem item : items) {
      if (!item.is_event_valid(this)) continue;
      item_count++;

      if (seen.insert(item.id).second && item.is_event_valid(this))
        new_items.push_back(item);
    }

    if (new_items.size() == item_count) new_items.clear();

    this->items = std::move(items);

    return new_items;
  }

  void RSSEventRepository::update_events() {
    data::DatabaseConnection conn = data::create_connection();

    data::DatabaseRows rows = conn->exec("SELECT event_type, name FROM events");

    std::unordered_set<std::string> keys;
    for (data::DatabaseRow row : rows) {
      std::string name = row.at("name");
      std::string type = row.at("event_type");
      keys.insert(type + '\0' + name);
    }

    this->events.erase(
        std::remove_if(
            this->events.begin(), this->events.end(),
            [&keys, this](const RSSEvent &e) {
              bool r =
                  keys.find(e.get_type() + '\0' + e.get_name()) == keys.end();
              if (r)
                this->logger.debug(std::format("Deleted event: {}:{}",
                                               e.get_name(), e.get_type()));

              return r;
            }),
        this->events.end());

    std::vector<RSSEvent> new_events;
    for (const data::DatabaseRow &row : rows) {
      const std::string &name = row.at("name");
      const std::string &type = row.at("event_type");

      const bool already_exists =
          std::any_of(this->events.begin(), this->events.end(),
                      [&name, &type](const RSSEvent &e) {
                        return e.get_name() == name && e.get_type() == type;
                      });

      if (!already_exists) {
        this->logger.debug(
            std::format("Created a new event: {}:{}", name, type));
        new_events.emplace_back(type, name);
      }
    }

    this->events.insert(this->events.begin(), new_events.begin(),
                        new_events.end());
  }

  void RSSEventRepository::poll() {
    auto &cfg = Configuration::get_instance();

    while (true) {
      try {
        this->update_events();
      } catch (std::exception e) {
        this->logger.error(e.what());
      }

      for (RSSEvent &e : this->events) {
        try {
          std::string url = e.get_url();
          cpr::Response response =
              cpr::Get(cpr::Url{url},
                       cpr::Header{{"Accept", "application/xml"},
                                   {"User-Agent", cfg.instance.user_agent},
                                   {"Cache-Control", "no-cache"},
                                   {"Pragma", "no-cache"}});

          this->logger.debug(std::format("Fetching {}...", url));

          if (response.status_code != 200) {
            this->logger.warn(std::format("{} returned {} status code", url,
                                          response.status_code));
            continue;
          }

          pugi::xml_document doc;
          if (!doc.load_string(response.text.c_str())) {
            this->logger.warn(std::format("{} returned {} status code", url,
                                          response.status_code));
            continue;
          }

          pugi::xml_node channel = doc.child("rss").child("channel");

          // parsing RSS items
          std::vector<RSSItem> items;
          for (pugi::xml_node i : channel.children("item")) {
            std::string title = i.child("title").text().as_string(),
                        link = i.child("link").text().as_string(),
                        author = i.child("author").text().as_string(),
                        origin = e.get_name();
            if (title.starts_with("Bridge returned error")) continue;

            // parsing timestamp
            long timestamp = 0;
            std::string pubdate = i.child("pubDate").text().as_string();
            pubdate = pubdate.substr(0, pubdate.size() - 6);
            std::tm tm = {};
            std::istringstream ss(pubdate);
            ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
            if (!ss.fail()) {
              timestamp = timegm(&tm);
            }

            std::vector<std::string> categories;

            for (pugi::xml_node i : i.children("category")) {
              categories.push_back(i.text().as_string());
            }

            RSSItem item = {i.child("guid").text().as_string(),
                            title,
                            link,
                            origin,
                            author,
                            categories,
                            timestamp};
            items.push_back(item);
          }

          std::vector<RSSItem> new_items = e.set_items(items);
          if (!new_items.empty()) {
            this->on_event_fn(e.get_type(), e.get_name(), new_items);
          }
        } catch (std::exception &e) {
          this->logger.error(e.what());
        }
      }

      std::this_thread::sleep_for(std::chrono::seconds(cfg.rss.timeout));
    }
  }

  void RSSEventRepository::on_event(
      std::function<void(const std::string &type, const std::string &name,
                         const std::vector<RSSItem> &items)>
          fn) {
    this->on_event_fn = fn;
  }
}
