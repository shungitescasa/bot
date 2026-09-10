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
      RPCClient() = default;
      RPCClient(std::string host, unsigned int port, unsigned int timeout = 0)
          : host(std::move(host)),
            port(port),
            timeout(timeout),
            log("ScriptVM-Client/" + host + ":" + std::to_string(port)) {
        connect();
      }
      RPCClient(const RPCClient &) = delete;
      RPCClient &operator=(const RPCClient &) = delete;

      bool is_alive();
      long long uptime();
      bool connect();
      bool connect(std::string host, unsigned int port,
                   unsigned int timeout = 0);

      bot::Response execute_untrusted_script(const std::string &script,
                                             const bot::Request &request);

      bot::Response execute(const bot::Request &request);

      bot::CommandDataVec list();

      static RPCClient &get_instance() {
        static RPCClient instance;
        return instance;
      }

    private:
      bot::Logger log;

      std::string host;
      unsigned int port, timeout;

      std::unique_ptr<rpc::client> client;
  };
}
