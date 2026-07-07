#include "scriptvm/client.hpp"

#include <optional>
#include <string>

namespace scriptvm {
  std::optional<std::string> RPCClient::execute_untrusted_script(
      const std::string &script) {
    return this->client.call("execute_untrusted_script", script)
        .as<std::optional<std::string>>();
  }
}