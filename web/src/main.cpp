#include "crow/app.h"
#include "crow/mustache.h"

int main(int argc, char *argv[]) {
  crow::SimpleApp app;

  CROW_ROUTE(app, "/")
  ([]() {
    auto page = crow::mustache::load("index.html");

    return page.render();
  });

  app.multithreaded().port(18083).run();
  return 0;
}
