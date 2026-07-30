#include "core/externalapi/twitch.hpp"

#include <cpr/cpr.h>

#include <algorithm>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/utils.hpp"

namespace bot::externalapi::twitch {
  void HelixClient::verify_token() {
    this->log.info("Verifying new token...");
    cpr::Response response = cpr::Get(
        cpr::Url{"https://id.twitch.tv/oauth2/validate"},
        cpr::Header{
            {"Authorization", "OAuth " + this->token},
            {"User-Agent", Configuration::get_instance().instance.user_agent}});

    if (response.status_code != 200) {
      throw std::runtime_error("Invalid Twitch OAuth2 token");
    }

    nlohmann::json j = nlohmann::json::parse(response.text);

    this->client_id = j["client_id"];
    this->login = j["login"];
    this->user_id = j["user_id"];
    this->log.info(std::format("Verified! Client ID: {}", this->client_id));
  }

  std::vector<User> HelixClient::get_users(
      const std::vector<std::string> &logins) const {
    return this->get_users({}, logins);
  }

  std::vector<User> HelixClient::get_users(const std::vector<int> &ids) const {
    return this->get_users(ids, {});
  }

  std::vector<User> HelixClient::get_users(
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

    return this->get_users_by_query("?" + utils::string::join(params, "&"));
  }

  std::vector<User> HelixClient::get_users_by_query(
      const std::string &query) const {
    Configuration &cfg = Configuration::get_instance();
    cpr::Response response = cpr::Get(
        cpr::Url{this->base_url + "/users" + query}, cpr::Bearer{this->token},
        cpr::Header{{"Client-Id", this->client_id.c_str()},
                    {"User-Agent", cfg.instance.user_agent}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<User> users;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      User u{d["login"], (unsigned int)std::stoi(d["id"].get<std::string>())};

      users.push_back(u);
    }

    return users;
  }

  std::vector<User> HelixClient::get_chatters(const int &broadcaster_id) const {
    Configuration &cfg = Configuration::get_instance();
    cpr::Response response =
        cpr::Get(cpr::Url{std::format(
                     "{}/chat/chatters?broadcaster_id={}&moderator_id={}",
                     this->base_url, broadcaster_id, this->user_id)},
                 cpr::Bearer{this->token},
                 cpr::Header{{"Client-Id", this->client_id.c_str()},
                             {"User-Agent", cfg.instance.user_agent}});

    if (response.status_code != 200) {
      return {};
    }

    std::vector<User> users;

    nlohmann::json j = nlohmann::json::parse(response.text);

    for (const auto &d : j["data"]) {
      User u{
          d["user_login"],
          (unsigned int)std::stoi(d["user_id"].get<std::string>()),
      };

      users.push_back(u);
    }

    return users;
  }

  std::vector<emotespp::Emote> HelixClient::get_global_emotes() const {
    Configuration &cfg = Configuration::get_instance();
    cpr::Response response =
        cpr::Get(cpr::Url{this->base_url + "/chat/emotes/global"},
                 cpr::Bearer{this->token},
                 cpr::Header{{"Client-Id", this->client_id.c_str()},
                             {"User-Agent", cfg.instance.user_agent}});

    if (response.status_code != 200) {
      throw std::runtime_error("Failed to get global emotes: " +
                               std::to_string(response.status_code));
    }

    nlohmann::json j = nlohmann::json::parse(response.text);
    std::vector<emotespp::Emote> emotes;

    for (const auto &d : j["data"]) {
      emotes.push_back({d["id"], d["name"]});
    }

    return emotes;
  }

  std::vector<emotespp::Emote> HelixClient::get_channel_emotes(
      const int &channel_id) const {
    Configuration &cfg = Configuration::get_instance();
    cpr::Response response = cpr::Get(
        cpr::Url{this->base_url +
                 "/chat/emotes?broadcaster_id=" + std::to_string(channel_id)},
        cpr::Bearer{this->token},
        cpr::Header{{"Client-Id", this->client_id.c_str()},
                    {"User-Agent", cfg.instance.user_agent}});

    if (response.status_code != 200) {
      throw std::runtime_error("Failed to get channel emotes: " +
                               std::to_string(response.status_code));
    }

    nlohmann::json j = nlohmann::json::parse(response.text);
    std::vector<emotespp::Emote> emotes;

    for (const auto &d : j["data"]) {
      emotes.push_back({d["id"], d["name"]});
    }

    return emotes;
  }
}
