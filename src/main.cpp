#include <iostream>

#include "irc/client.hpp"

int main(int argc, char *argv[]) {
  std::cout << "hi world\n";

  RedpilledBot::IRC::Client client("", "");

  client.run();

  return 0;
}
