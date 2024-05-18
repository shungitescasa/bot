#include "crow/app.h"

int main(int argc, char *argv[]) {
  crow::SimpleApp app;

  CROW_ROUTE(app, "/")([]() { return "hi"; });

  app.multithreaded().port(18083).run();
  return 0;
}
