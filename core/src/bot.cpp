#include "core/bot.hpp"

#include <string>
#include <vector>

#include "core/externalapi/twitch.hpp"
#include "core/message.hpp"

namespace bot {
  void RPCChatBotServer::run() {
    this->server.bind("alive", [&]() {
      this->log.debug("Alive!");
      return true;
    });

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

    this->server.bind("bot_me", [&]() { return this->bot->get_me(); });

    this->server.bind("twitch_get_chatters", [&](const int &broadcaster_id) {
      return externalapi::twitch::HelixClient::get_instance().get_chatters(
          broadcaster_id);
    });

    this->server.bind(
        "twitch_get_users", [&](const std::vector<int> &ids,
                                const std::vector<std::string> &logins) {
          return externalapi::twitch::HelixClient::get_instance().get_users(
              ids, logins);
        });

    this->server.bind("twitch_get_global_emotes", [&]() {
      auto &api = externalapi::twitch::HelixClient::get_instance();
      auto emotes = api.get_global_emotes();
      std::vector<externalapi::twitch::MsgPackEmote> em;
      for (auto &e : emotes) {
        em.push_back({e});
      }
      return em;
    });

    this->server.bind(
        "twitch_get_channel_emotes", [&](const int &broadcaster_id) {
          auto &api = externalapi::twitch::HelixClient::get_instance();
          auto emotes = api.get_channel_emotes(broadcaster_id);
          std::vector<externalapi::twitch::MsgPackEmote> em;
          for (auto &e : emotes) {
            em.push_back({e});
          }
          return em;
        });

    this->log.info(std::format("Starting RPC chatbot server on port {}...",
                               this->server.port()));
    this->server.run();
  }

  bool RPCChatBot::is_alive() {
    if (!this->client || this->client->get_connection_state() ==
                             rpc::client::connection_state::disconnected) {
      return this->connect();
    }

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

  MessageSource RPCChatBot::get_me() {
    if (!this->is_alive()) return {};
    return this->client->call("bot_me").as<MessageSource>();
  }

  std::vector<externalapi::twitch::User> RPCChatBot::get_chatters(
      const int &broadcaster_id) {
    if (!this->is_alive()) return {};
    return this->client->call("twitch_get_chatters", broadcaster_id)
        .as<std::vector<externalapi::twitch::User>>();
  }

  std::vector<externalapi::twitch::User> RPCChatBot::get_users(
      const std::vector<int> &ids, const std::vector<std::string> &logins) {
    if (!this->is_alive()) return {};
    return this->client->call("twitch_get_users", ids, logins)
        .as<std::vector<externalapi::twitch::User>>();
  }

  std::vector<externalapi::twitch::MsgPackEmote>
  RPCChatBot::get_global_emotes() {
    if (!this->is_alive()) return {};
    return this->client->call("twitch_get_global_emotes")
        .as<std::vector<externalapi::twitch::MsgPackEmote>>();
  }

  std::vector<externalapi::twitch::MsgPackEmote> RPCChatBot::get_channel_emotes(
      const int &id) {
    if (!this->is_alive()) return {};
    return this->client->call("twitch_get_channel_emotes", id)
        .as<std::vector<externalapi::twitch::MsgPackEmote>>();
  }
}
