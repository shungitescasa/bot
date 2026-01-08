#include "rss.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <optional>
#include <pugixml.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "database.hpp"
#include "fmt/format.h"
#include "logger.hpp"
#include "schemas/event.hpp"
#include "schemas/stream.hpp"
#include "utils/events.hpp"
#include "utils/string.hpp"

namespace bot {
  sol::table RSSChannel::as_lua_table(std::shared_ptr<sol::state> state) const {
    sol::table t = state->create_table();
    t["name"] = this->name;
    t["url"] = this->url;

    if (this->event.has_value()) {
      sol::table e = state->create_table();
      e["name"] = this->event->name;
      e["type"] = this->event->type;
      t["event"] = e;
    } else {
      t["event"] = sol::lua_nil;
    }

    sol::table ms = state->create_table();

    for (const RSSMessage &v : this->messages) {
      sol::table m = state->create_table();
      m["message"] = v.message;
      m["id"] = v.id;
      m["timestamp"] = v.timestamp;
      ms.add(m);
    }

    t["messages"] = ms;

    return t;
  }

  void RSSListener::run() {
    if (!this->configuration.rss.bridge.has_value()) {
      log::error("RSSListener", "RSS Bridge is not set!");
      return;
    }

    while (true) {
      this->add_channels();
      this->check_channels();
      std::this_thread::sleep_for(
          std::chrono::seconds(this->configuration.rss.timeout));
    }
  }

  bool RSSListener::has_channel(const std::string &url) const {
    return std::any_of(this->channels.begin(), this->channels.end(),
                       [&url](const RSSChannel &c) { return c.url == url; });
  }

  void RSSListener::add_channel(const std::string &url) {
    if (this->has_channel(url)) return;

    std::optional<RSSChannel> channel = get_rss_channel(url);
    if (channel.has_value()) {
      this->channels.push_back(channel.value());
    }
  }

  void RSSListener::remove_channel(const std::string &url) {
    if (!this->has_channel(url)) return;

    std::remove_if(this->channels.begin(), this->channels.end(),
                   [&url](const RSSChannel &c) { return c.url == url; });
  }

  void RSSListener::add_channels() {
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(this->configuration);

    db::DatabaseRows events = conn->exec(
        "SELECT event_type, name "
        "FROM events "
        "WHERE event_type BETWEEN $1 AND $2",
        {std::to_string(static_cast<int>(schemas::EventType::RSS)),
         std::to_string(static_cast<int>(schemas::EventType::TELEGRAM))});

    // adding new events
    for (db::DatabaseRow event : events) {
      std::string name = event.at("name");
      int type = std::stoi(event.at("event_type"));
      std::string bridge = "";
      bool useRSSBridge = true;

      switch (type) {
        case schemas::RSS:
          bridge = "RSS";
          useRSSBridge = false;
          break;
        case schemas::TWITTER:
          bridge = "FarsideNitterBridge";
          break;
        case schemas::TELEGRAM:
          bridge = "TelegramBridge";
          break;
        default:
          break;
      }

      if (bridge.empty()) {
        log::warn("RSSListener",
                  "Failed to specify bridge: " + event.at("event_type"));
        continue;
      }

      if (std::any_of(this->channels.begin(), this->channels.end(),
                      [&name, &type](const RSSChannel &c) {
                        auto e = c.event;
                        if (!e.has_value()) {
                          return false;
                        }
                        return e->name == name && e->type == type;
                      })) {
        continue;
      }

      std::string url =
          useRSSBridge
              ? fmt::format(*this->configuration.rss.bridge, bridge, name)
              : name;

      std::optional<RSSChannel> channel = get_rss_channel(url);
      if (!channel.has_value()) {
        log::warn("RSSListener", "No RSS feed on " + url);
        continue;
      }

      channel->event = {name, type};

      this->channels.push_back(*channel);
    }

    // removing old events
    auto channels = this->channels;
    for (RSSChannel c : channels) {
      if (!c.event.has_value()) {
        continue;
      }

      if (!std::any_of(events.begin(), events.end(),
                       [&c](const db::DatabaseRow &r) {
                         return r.at("name") == c.event->name &&
                                std::stoi(r.at("event_type")) == c.event->type;
                       })) {
        this->remove_channel(c.url);
      }
    }
  }

  void RSSListener::check_channels() {
    for (auto it = this->channels.begin(); it != this->channels.end(); ++it) {
      if (!it->event.has_value()) {
        continue;
      }

      std::optional<RSSChannel> channel = get_rss_channel(it->url);
      if (!channel.has_value()) {
        continue;
      }

      std::vector<RSSMessage> messages;
      for (auto mit = channel->messages.begin(); mit != channel->messages.end();
           ++mit) {
        if (!std::any_of(
                it->messages.begin(), it->messages.end(),
                [&mit](const RSSMessage &m) { return m.id == mit->id; })) {
          messages.push_back(*mit);
        }
      }

      if (messages.empty()) {
        continue;
      }

      // getting channels
      std::vector<schemas::Event> events = utils::get_events(
          db::create_connection(this->configuration), this->helix_client,
          this->irc_client.get_me().id, it->event->type, it->event->name);

      for (const schemas::Event &event : events) {
        int count = 0;

        for (RSSMessage message : messages) {
          if (count > 5) break;
          count++;

          std::string base = "⚡ " + event.message;
          if (!event.subs.empty()) {
            base.append(" · ");
          }

          int pos = base.find("{channel_name}");
          if (pos != std::string::npos) base.replace(pos, 14, it->name);

          pos = base.find("{message}");
          if (pos != std::string::npos) base.replace(pos, 9, message.message);

          pos = base.find("{link}");
          if (pos != std::string::npos) base.replace(pos, 6, message.id);

          std::vector<std::string> msgs = utils::string::separate_by_length(
              base, event.subs, "@", " ", 500);

          for (const std::string &msg : msgs) {
            this->irc_client.say(
                {event.channel_alias_name, event.channel_alias_id}, base + msg);
          }
        }
      }

      it->messages = channel->messages;
    }
  }

  std::optional<RSSChannel> get_rss_channel(const std::string &url) {
    cpr::Response response =
        cpr::Get(cpr::Url{url}, cpr::Header{{"Accept", "application/xml"},
                                            {"User-Agent", "Mozilla/5.0"},
                                            {"Cache-Control", "no-cache"},
                                            {"Pragma", "no-cache"}});

    if (response.status_code != 200) {
      return std::nullopt;
    }

    pugi::xml_document doc;
    if (!doc.load_string(response.text.c_str())) {
      return std::nullopt;
    }

    pugi::xml_node channel = doc.child("rss").child("channel");

    std::string channel_name = channel.child("title").text().as_string();

    std::vector<RSSMessage> messages;
    for (pugi::xml_node item : channel.children("item")) {
      // parsing timestamp
      long timestamp = 0;
      std::string pubdate = item.child("pubDate").text().as_string();
      pubdate = pubdate.substr(0, pubdate.size() - 6);
      std::tm tm = {};
      std::istringstream ss(pubdate);
      ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
      if (!ss.fail()) {
        timestamp = timegm(&tm);
      }

      RSSMessage message = {item.child("guid").text().as_string(),
                            item.child("title").text().as_string(), timestamp};

      if (message.message.find("Bridge returned error") != std::string::npos) {
        log::warn("RSSListener/" + url, "Bridge returned error");
        messages.clear();
        break;
      }

      messages.push_back(message);
    }

    return (RSSChannel){channel_name, url, std::nullopt, messages};
  }
}