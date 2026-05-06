#ifdef IPC_SERVER
#pragma once
#include <unistd.h>

#include <string>

#include "chat.hpp"

namespace bot {
  class IPCServer {
    public:
      IPCServer(const std::string &path, chat::ChatClient &chat_client)
          : path(path), chat_client(chat_client) {}
      ~IPCServer() = default;

      void connect();
      void run();

    private:
      std::string process_message(const std::string &message);

      const std::string &path;
      chat::ChatClient &chat_client;
      int server_fd = -1;
  };
}
#endif