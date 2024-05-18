#include "config.hpp"
#include "crow/app.h"
#include "crow/mustache.h"
#include "handlers.hpp"

int main(int argc, char *argv[]) {
  auto cfg = botweb::parse_configuration_from_file(".env");

  crow::SimpleApp app;

  CROW_ROUTE(app, "/")
  ([&cfg]() {
    auto page = crow::mustache::load("index.html");

    crow::mustache::context ctx;
    ctx["contact_url"] = (*cfg).contact_url;
    ctx["contact_name"] = (*cfg).contact_name;

    return page.render(ctx);
  });

  CROW_ROUTE(app, "/wiki")
  ([&cfg]() { return botweb::get_wiki_page("/README", cfg); });
  CROW_ROUTE(app, "/wiki/<path>")
  ([&cfg](const std::string &path) {
    return botweb::get_wiki_page(path, cfg);
  });

  app.multithreaded().port(18083).run();
  return 0;
}
