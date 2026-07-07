#include "scriptvm/client.hpp"

#include <format>
#include <memory>
#include <optional>
#include <string>

#include "core/command.hpp"
#include "rpc/client.h"

namespace scriptvm {
  std::optional<std::string> RPCClient::execute_untrusted_script(
      const std::string &script) {
    if (!this->is_alive()) return std::nullopt;
    return this->client->call("execute_untrusted_script", script)
        .as<std::optional<std::string>>();
  }

  bot::Response RPCClient::execute(const bot::Request &request) {
    if (!this->is_alive()) return {};
    return this->client->call("exec", request).as<bot::Response>();
  }

  bot::CommandDataVec RPCClient::list() {
    if (!this->is_alive()) return {};
    return this->client->call("list").as<bot::CommandDataVec>();
  }

  bool RPCClient::connect() {
    try {
      this->log.info(
          std::format("Connecting to {}:{}...", this->host, this->port));
      this->client = std::make_unique<rpc::client>(this->host, this->port);
      if (this->timeout) this->client->set_timeout(this->timeout);
      return true;
    } catch (...) {
      this->client.reset();
      return false;
    }
  }

  bool RPCClient::is_alive() {
    if (!this->client || this->client->get_connection_state() ==
                             rpc::client::connection_state::disconnected)
      return false;

    try {
      return this->client->call("alive").as<bool>();
    } catch (...) {
      return false;
    }
  }
}