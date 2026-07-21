#include "core/bot.hpp"

#include "core/message.hpp"

namespace bot {
  void RPCChatBotServer::run() {
    this->server.bind("alive", []() { return true; });

    this->server.bind("bot_send_message", [&](const MessageSource &source,
                                              const std::string &message) {
      this->bot->send_message(source.login, message);
    });

    this->server.bind("bot_part", [&](const MessageSource &source,
                                      const std::string &message) {
      this->bot->part(source);
    });

    this->server.bind("bot_join", [&](const MessageSource &source,
                                      const std::string &message) {
      this->bot->join(source);
    });

    this->server.async_run();
  }

  bool RPCChatBot::is_alive() {
    if (!this->client || this->client->get_connection_state() ==
                             rpc::client::connection_state::disconnected)
      return false;

    try {
      return this->client->call("alive").as<bool>();
    } catch (...) {
      return false;
    }
  }

  bool RPCChatBot::connect() {
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

  bool RPCChatBot::connect(std::string host, unsigned int port,
                           unsigned int timeout) {
    this->host = host;
    this->port = port;
    this->timeout = timeout;
    this->log = {"TinyBot-RPCChatBot/" + host + ":" + std::to_string(port)};
    return this->connect();
  }

  void RPCChatBot::send_message(const MessageSource &source,
                                const std::string &message) {
    if (!this->is_alive()) return;
    this->client->send("bot_send_message", source, message);
  }

  void RPCChatBot::join(const MessageSource &source) {
    if (!this->is_alive()) return;
    this->client->send("bot_join", source);
  }

  void RPCChatBot::part(const MessageSource &source) {
    if (!this->is_alive()) return;
    this->client->send("bot_part", source);
  }
}
