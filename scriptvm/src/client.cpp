#include "scriptvm/client.hpp"

#include <optional>
#include <string>

#include "core/command.hpp"

namespace scriptvm {
  std::optional<std::string> RPCClient::execute_untrusted_script(
      const std::string &script) {
    return this->client.call("execute_untrusted_script", script)
        .as<std::optional<std::string>>();
  }

  bot::Response RPCClient::execute(const bot::Request &request) {
    return this->client.call("exec", request).as<bot::Response>();
  }

  bot::CommandDataVec RPCClient::list() {
    return this->client.call("list").as<bot::CommandDataVec>();
  }
}