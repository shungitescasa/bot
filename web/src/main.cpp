#include "config.hpp"
#include "crow/app.h"
#include "crow/mustache.h"
#include "handlers.hpp"

int main(int argc, char *argv[]) {
  auto cfg = botweb::parse_configuration_from_file(".env");

  crow::SimpleApp app;

  CROW_ROUTE(app, "/")
  ([]() {
    auto page = crow::mustache::load("index.html");

    return page.render();
  });

  CROW_ROUTE(app, "/wiki")([]() { return botweb::get_wiki_page("/README"); });
  CROW_ROUTE(app, "/wiki/<path>")
  ([](const std::string &path) { return botweb::get_wiki_page(path); });

  app.multithreaded().port(18083).run();
  return 0;
}
