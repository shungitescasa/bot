#ifdef IPC_SERVER
#include "ipc.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

#include "fmt/format.h"
#include "logger.hpp"
#include "nlohmann/json.hpp"

namespace bot {
  void IPCServer::connect() {
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
      throw std::runtime_error("Failed to start the IPC server");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, this->path.c_str(), sizeof(addr.sun_path) - 1);

    unlink(this->path.c_str());

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
      throw std::runtime_error("Failed to bind the IPC server");
    }

    if (listen(this->server_fd, 5) < 0) {
      throw std::runtime_error("Failed to prepare connections for IPC server");
    }
  }

  void IPCServer::run() {
    if (this->server_fd < 0) {
      throw std::runtime_error("Failed to run the IPC server");
    }

    log::info("IPCServer", "Accepting connections...");

    while (true) {
      int client = accept(this->server_fd, nullptr, nullptr);
      if (client < 0) {
        continue;
      }

      char buffer[1024]{};
      int n = read(client, buffer, sizeof(buffer) - 1);

      if (n > 0) {
        buffer[n] = '\0';
        std::string message(buffer);
        log::debug("IPCServer", fmt::format("Received: {}", message));

        std::string response = this->process_message(message);

        write(client, fmt::format("{}\r\n", response).c_str(),
              response.length() + 2);
      }

      close(client);
    }
  }

  std::string IPCServer::process_message(const std::string &message) {
    nlohmann::json j = nlohmann::json::parse(message);
    nlohmann::json resp = {{"operation", "acknowledge"}, {"data", nullptr}};

    if (!j.contains("operation") || !j.contains("data")) {
      resp["operation"] = "error";
      resp["data"] = "invalid payload";
      return resp.dump();
    }

    std::string op = j["operation"];
    nlohmann::json d = j["data"];

    if (op == "channel.join") {
      std::string alias_name = d["alias_name"];
      int alias_id = d["alias_id"];

      this->chat_client.join({alias_name, alias_id});
    }

    return resp.dump();
  }
}
#endif