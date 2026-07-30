#pragma once

#include <emotespp/emotes.hpp>
#include <string>
#include <vector>

#include "core/log.hpp"
#include "rpc/msgpack.hpp"

namespace bot::externalapi::twitch {
  struct User {
      std::string login = "";
      unsigned int id = 0;

      MSGPACK_DEFINE(login, id);
  };

  struct MsgPackEmote : emotespp::Emote {
      MsgPackEmote() = default;

      MsgPackEmote(const Emote &e) {
        this->id = e.id;
        this->code = e.code;
        this->original_code = e.original_code;
      }

      MSGPACK_DEFINE(id, code, original_code);
  };

  class HelixClient {
    public:
      HelixClient() : log("HelixClient") {};
      HelixClient(const std::string &token) : token(token), log("HelixClient") {
        verify_token();
      };
      ~HelixClient() = default;
      HelixClient(const HelixClient &) = delete;
      HelixClient &operator=(const HelixClient &) = delete;

      std::vector<User> get_users(const std::vector<std::string> &logins) const;
      std::vector<User> get_users(const std::vector<int> &ids) const;
      std::vector<User> get_users(const std::vector<int> &ids,
                                  const std::vector<std::string> logins) const;

      std::vector<User> get_chatters(const int &broadcaster_id) const;

      std::vector<emotespp::Emote> get_global_emotes() const;
      std::vector<emotespp::Emote> get_channel_emotes(
          const int &channel_id) const;

      void set_token(const std::string &token) {
        this->token = token;
        this->verify_token();
      }

      static HelixClient &get_instance() {
        static HelixClient instance;
        return instance;
      }

    private:
      void verify_token();
      std::vector<User> get_users_by_query(const std::string &query) const;

      bot::Logger log;

      std::string token, client_id, login, user_id;

      const std::string base_url = "https://api.twitch.tv/helix";
  };
}
