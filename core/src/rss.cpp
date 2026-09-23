#include "core/rss.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <map>
#include <sol/table.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "core/config.hpp"
#include "core/data/database.hpp"
#include "core/utils.hpp"
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
        (type == "7tv.added-emote" && has_category("7TV") &&
         has_category("New")) ||
        (type == "7tv.deleted-emote" && has_category("7TV") &&
         has_category("Deleted")) ||
        (type == "7tv.renamed-emote" && has_category("7TV") &&
         has_category("Renamed")) ||

        // BetterTTV
        (type == "bttv.added-emote" && has_category("BetterTTV") &&
         has_category("New")) ||
        (type == "bttv.deleted-emote" && has_category("BetterTTV") &&
         has_category("Deleted")) ||
        (type == "bttv.renamed-emote" && has_category("BetterTTV") &&
         has_category("Renamed")) ||

        // GitHub
        (type == "github.commit" && id.starts_with("tag:github.com")) ||

        // Twitter
        (type == "twitter.post" &&
         this->link.starts_with("https://twitter.com/")) ||

        // Telegram
        (type == "telegram.post" && this->link.starts_with("https://t.me/")) ||

        // RSS
        type == "rss";
  }

  sol::table RSSItem::as_lua_table(std::shared_ptr<sol::state> state) const {
    sol::table o = state->create_table();
    o["id"] = id;
    o["title"] = title;
    o["link"] = link;
    o["origin"] = origin;
    o["author"] = author;
    o["timestamp"] = timestamp;
    sol::table c = state->create_table();
    for (std::string x : categories) c.add(x);
    o["categories"] = c;
    return o;
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
    } else if (this->type == "github.commit") {
      url << "https://github.com/";

      int pos = name.find("/");
      if (pos == std::string::npos) {
        throw std::runtime_error(
            "Invalid GitHub target (not username/repository format)");
      }

      url << name.substr(0, pos);
      url << "/";

      std::string rest = name.substr(pos + 1);
      std::string branch = "master";

      pos = rest.find("/");
      if (pos != std::string::npos) {
        url << rest.substr(0, pos);
        branch = rest.substr(pos + 1);
      } else {
        url << rest;
      }

      url << "/commits/";
      url << branch;
      url << ".atom";
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
      url << "TwitchEmoteUpdatesBridge&provider=stv";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("bttv.")) {
      url << "TwitchEmoteUpdatesBridge&provider=bttv";
      url << "&channel=" << this->name;
    } else if (this->type.starts_with("twitter.")) {
      url << "FarsideNitterBridge";
      url << "&noreply=on&noretweet=on&linkbacktotwitter=on";
      url << "&username=" << this->name;
    } else if (this->type.starts_with("telegram.")) {
      url << "TelegramBridge";
      url << "&username=%40" << this->name;
    }

    else if (this->type != "rss" && this->type != "github.commit") {
      throw std::runtime_error("Unsupported event type");
    }

    this->url = url.str();
  }

  std::vector<RSSItem> RSSEvent::fetch_items() const {
    auto &cfg = Configuration::get_instance();

    std::string url = this->get_url();
    cpr::Response response = cpr::Get(
        cpr::Url{url}, cpr::Header{{"Accept", "application/xml"},
                                   {"User-Agent", cfg.instance.user_agent},
                                   {"Cache-Control", "no-cache"},
                                   {"Pragma", "no-cache"}});

    if (response.status_code != 200) {
      throw std::runtime_error(
          std::format("{} returned {} status code", url, response.status_code));
    }

    pugi::xml_document doc;
    if (!doc.load_string(response.text.c_str())) {
      throw std::runtime_error("Not valid XML format");
    }

    pugi::xml_node rss = doc.child("rss");
    pugi::xml_node feed = doc.child("feed");

    if (rss) {
      return this->parse_rss_feed(rss);
    } else if (feed) {
      return this->parse_atom_feed(feed);
    }

    throw std::runtime_error("Unrecognized feed type");
  }

  std::vector<RSSItem> RSSEvent::parse_rss_feed(
      const pugi::xml_node &feed) const {
    pugi::xml_node channel = feed.child("channel");

    // parsing RSS items
    std::vector<RSSItem> items;
    for (pugi::xml_node i : channel.children("item")) {
      std::string title =
                      utils::string::trim(i.child("title").text().as_string()),
                  link = i.child("link").text().as_string(),
                  author = i.child("author").text().as_string(),
                  origin = this->get_name();
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

    return items;
  }

  std::vector<RSSItem> RSSEvent::parse_atom_feed(
      const pugi::xml_node &feed) const {
    std::vector<RSSItem> items;

    for (pugi::xml_node i : feed.children("entry")) {
      std::string title =
                      utils::string::trim(i.child("title").text().as_string()),
                  origin = this->get_name();
      if (title.starts_with("Bridge returned error")) continue;

      std::string link;
      for (pugi::xml_node l : i.children("link")) {
        std::string rel = l.attribute("rel").as_string("alternate");
        if (rel == "alternate") {
          link = l.attribute("href").as_string();
          break;
        }
        if (link.empty()) link = l.attribute("href").as_string();
      }

      std::string author;
      pugi::xml_node authorNode = i.child("author");
      if (authorNode) {
        pugi::xml_node nameNode = authorNode.child("name");
        if (nameNode) {
          author = nameNode.text().as_string();
        } else {
          author = authorNode.text().as_string();
        }
      }

      long timestamp = 0;
      std::string date = i.child("published").text().as_string();
      if (date.empty()) {
        date = i.child("updated").text().as_string();
      }
      if (!date.empty()) {
        if (!date.empty() && (date.back() == 'Z')) {
          date.pop_back();
        } else if (date.size() > 6 && (date[date.size() - 6] == '+' ||
                                       date[date.size() - 6] == '-')) {
          date = date.substr(0, date.size() - 6);
        }

        std::tm tm = {};
        std::istringstream ss(date);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (!ss.fail()) {
          timestamp = timegm(&tm);
        }
      }

      std::vector<std::string> categories;
      for (pugi::xml_node c : i.children("category")) {
        std::string term = c.attribute("term").as_string();
        if (!term.empty()) categories.push_back(term);
      }

      RSSItem item = {i.child("id").text().as_string(),
                      title,
                      link,
                      origin,
                      author,
                      categories,
                      timestamp};
      items.push_back(item);
    }

    return items;
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

    data::DatabaseRows rows = conn->exec(
        "SELECT DISTINCT e.event_type, e.name FROM events e "
        "INNER JOIN rooms r ON r.id = e.room_id "
        "WHERE r.parted_at IS NULL");

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
      } catch (std::runtime_error e) {
        this->logger.error(e.what());
      }

      std::map<std::string, std::vector<RSSItem>> cached_items;

      for (RSSEvent &e : this->events) {
        try {
          std::vector<RSSItem> new_items;

          if (cached_items.contains(e.get_url())) {
            new_items = cached_items.at(e.get_url());
          } else {
            this->logger.debug(std::format("Fetching {}...", e.get_url()));
            new_items = e.set_items(e.fetch_items());
            cached_items.insert({e.get_url(), new_items});
          }

          if (!new_items.empty()) {
            this->on_event_fn(e.get_type(), e.get_name(), new_items);
          }
        } catch (std::runtime_error &e) {
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
