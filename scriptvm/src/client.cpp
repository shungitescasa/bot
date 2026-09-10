#include "scriptvm/client.hpp"

#include <format>
#include <memory>
#include <string>

#include "core/command.hpp"
#include "rpc/client.h"

namespace scriptvm {
  bot::Response RPCClient::execute_untrusted_script(
      const std::string &script, const bot::Request &request) {
    if (!this->is_alive()) return {};
    return this->client->call("untrusted_exec", script, request)
        .as<bot::Response>();
  }

  bot::Response RPCClient::execute(const bot::Request &request) {
    if (!this->is_alive()) return {};
    return this->client->call("exec", request).as<bot::Response>();
  }

  bot::CommandDataVec RPCClient::list() {
    if (!this->is_alive()) return {};
    return this->client->call("list").as<bot::CommandDataVec>();
  }

  bool RPCClient::connect(std::string host, unsigned int port,
                          unsigned int timeout) {
    this->host = host;
    this->port = port;
    this->timeout = timeout;
    this->log = {"ScriptVM-Client/" + host + ":" + std::to_string(port)};
    return this->connect();
  }

  bool RPCClient::connect() {
    if (this->host.empty()) {
      this->log.warn("No host provided");
      return false;
    }

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

  long long RPCClient::uptime() {
    if (!this->is_alive()) return -1;
    return this->client->call("uptime").as<long long>();
  }
}
