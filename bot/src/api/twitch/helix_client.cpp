#include "helix_client.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "cpr/api.h"
#include "cpr/bearer.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "schemas/stream.hpp"
#include "schemas/user.hpp"
#include "utils/string.hpp"

namespace bot::api::twitch {
  HelixClient::HelixClient(const std::string &token,
                           const std::string &client_id) {
    this->token = token;
    this->client_id = client_id;
  }

  std::vector<schemas::User> HelixClient::get_users(
      const std::vector<std::string> &logins) const {
    return this->get_users({}, logins);
  }

  std::vector<schemas::User> HelixClient::get_users(
      const std::vector<int> &ids) const {
    return this->get_users(ids, {});
  }

  std::vector<schemas::User> HelixClient::get_users(
      const std::vector<int> &ids,
      const std::vector<std::string> logins) const {
    std::vector<std::string> params;

    std::for_each(ids.begin(), ids.end(), [&params](const int &id) {
      params.push_back("id=" + std::to_string(id));
    });

    std::for_each(logins.begin(), logins.end(),
                  [&params](const std::string &login) {
                    params.push_back("login=" + login);
                  });

    return this->get_users_by_query("?" +
                                    utils::string::join_vector(params, '&'));
  }

  std::vector<schemas::User> HelixClient::get_users_by_query(
      const std::string &query) const {
    cpr::Response response = cpr::Get(
        cpr::Url{this->base_url + "/users" + query}, cpr::Bearer{this->token},
        cpr::Header{{"Client-Id", this->client_id.c_str()}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<schemas::User> users;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      schemas::User u{std::stoi(d["id"].get<std::string>()), d["login"]};

      users.push_back(u);
    }

    return users;
  }

  std::vector<schemas::User> HelixClient::get_chatters(
      const int &broadcaster_id, const int &moderator_id) const {
    cpr::Response response =
        cpr::Get(cpr::Url{this->base_url + "/chat/chatters?broadcaster_id=" +
                          std::to_string(broadcaster_id) +
                          "&moderator_id=" + std::to_string(moderator_id)},
                 cpr::Bearer{this->token},
                 cpr::Header{{"Client-Id", this->client_id.c_str()}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<schemas::User> users;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      schemas::User u{std::stoi(d["user_id"].get<std::string>()),
                      d["user_login"]};

      users.push_back(u);
    }

    return users;
  }

  std::vector<schemas::Stream> HelixClient::get_streams(
      const std::vector<int> &ids) const {
    std::string s;

    for (auto i = ids.begin(); i != ids.end(); i++) {
      std::string start;
      if (i == ids.begin()) {
        start = "?";
      } else {
        start = "&";
      }

      s += start + "user_id=" + std::to_string(*i);
    }

    cpr::Response response = cpr::Get(
        cpr::Url{this->base_url + "/streams" + s}, cpr::Bearer{this->token},
        cpr::Header{{"Client-Id", this->client_id.c_str()}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<schemas::Stream> streams;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      schemas::Stream u{std::stoi(d["user_id"].get<std::string>()),
                        d["user_login"], d["game_name"], d["title"],
                        d["started_at"]};

      streams.push_back(u);
    }

    return streams;
  }

  std::vector<schemas::Stream> HelixClient::get_channel_information(
      const std::vector<int> &ids) const {
    std::vector<std::string> s;

    for (const int &id : ids) {
      s.push_back("broadcaster_id=" + std::to_string(id));
    }

    cpr::Response response =
        cpr::Get(cpr::Url{this->base_url + "/channels?" +
                          utils::string::join_vector(s, '&')},
                 cpr::Bearer{this->token},
                 cpr::Header{{"Client-Id", this->client_id.c_str()}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<schemas::Stream> streams;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      schemas::Stream u{std::stoi(d["broadcaster_id"].get<std::string>()),
                        d["broadcaster_login"], d["game_name"], d["title"]};

      streams.push_back(u);
    }

    return streams;
  }
}
