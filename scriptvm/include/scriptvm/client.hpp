#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "core/command.hpp"
#include "core/log.hpp"
#include "rpc/client.h"

namespace scriptvm {
  class RPCClient {
    public:
      RPCClient(std::string host, unsigned int port, unsigned int timeout = 0)
          : host(std::move(host)),
            port(port),
            timeout(timeout),
            log("ScriptVM-Client/" + host + ":" + std::to_string(port)) {
        connect();
      }

      bool is_alive();
      bool connect();

      std::optional<std::string> execute_untrusted_script(
          const std::string &script);

      bot::Response execute(const bot::Request &request);

      bot::CommandDataVec list();

    private:
      const bot::Logger log;

      const std::string host;
      const unsigned int port, timeout;

      std::unique_ptr<rpc::client> client;
  };
}