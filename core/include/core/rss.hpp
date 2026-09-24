#pragma once

#include <functional>
#include <memory>
#include <sol/table.hpp>
#include <string>
#include <vector>

#include "core/log.hpp"
#include "pugixml.hpp"

namespace bot {
  class RSSEvent;

  struct RSSItem {
      std::string id = "", title = "", link = "", origin = "", author = "";
      std::vector<std::string> categories;
      long timestamp = 0;

      bool has_category(const std::string &) const;
      bool is_event_valid(RSSEvent *e) const;

      sol::table as_lua_table(std::shared_ptr<sol::state> state) const;
  };

  class RSSEvent {
    public:
      RSSEvent(const std::string &type, const std::string &name);
      ~RSSEvent() = default;

      const std::string &get_type() const;
      const std::string &get_name() const;
      const std::string &get_url() const;

      std::vector<RSSItem> set_items(std::vector<RSSItem> items);

      std::vector<RSSItem> fetch_items() const;

    private:
      std::vector<RSSItem> parse_rss_feed(const pugi::xml_node &feed) const;
      std::vector<RSSItem> parse_atom_feed(const pugi::xml_node &feed) const;

      std::string type, name, url;
      std::vector<RSSItem> items;
  };

  class RSSEventRepository {
    public:
      RSSEventRepository() : logger("RSSEventRepository") {};
      ~RSSEventRepository() = default;

      void poll();
      void update_events();
      void on_event(
          std::function<void(const std::string &type, const std::string &name,
                             const std::vector<RSSItem> &items)>
              fn);

      int event_count() const;

    private:
      std::vector<RSSEvent> events;
      std::function<void(const std::string &type, const std::string &name,
                         const std::vector<RSSItem> &items)>
          on_event_fn;
      bot::Logger logger;
  };
}
