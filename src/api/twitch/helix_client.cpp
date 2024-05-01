#include "helix_client.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "cpr/api.h"
#include "cpr/bearer.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "schemas/user.hpp"

namespace bot::api::twitch {
  HelixClient::HelixClient(const std::string &token,
                           const std::string &client_id) {
    this->token = token;
    this->client_id = client_id;
  }

  std::vector<schemas::User> HelixClient::get_users(
      const std::vector<std::string> &logins) const {
    std::string s;

    for (auto i = logins.begin(); i != logins.end(); i++) {
      std::string start;
      if (i == logins.begin()) {
        start = "?";
      } else {
        start = "&";
      }

      s += start + "login=" + *i;
    }

    return this->get_users_by_query(s);
  }

  std::vector<schemas::User> HelixClient::get_users(
      const std::vector<int> &ids) const {
    std::string s;

    for (auto i = ids.begin(); i != ids.end(); i++) {
      std::string start;
      if (i == ids.begin()) {
        start = "?";
      } else {
        start = "&";
      }

      s += start + "id=" + std::to_string(*i);
    }

    return this->get_users_by_query(s);
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
}
