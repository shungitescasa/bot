#include <iostream>

#include "irc/client.hpp"

int main(int argc, char *argv[]) {
  std::cout << "hi world\n";

  bot::irc::Client client("", "");

  client.run();

  return 0;
}
