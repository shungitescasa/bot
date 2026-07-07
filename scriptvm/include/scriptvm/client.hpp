#pragma once

#include <optional>
#include <string>

#include "rpc/client.h"

namespace scriptvm {
  class RPCClient {
    public:
      RPCClient(std::string host, unsigned int port) : client(host, port) {}

      std::optional<std::string> execute_untrusted_script(
          const std::string &script);

    private:
      rpc::client client;
  };
}