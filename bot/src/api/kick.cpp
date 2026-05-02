#include "api/kick.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "utils/chrono.hpp"
#include "utils/string.hpp"

namespace bot::api {
  std::vector<KickChannel> parse_json_channels(const nlohmann::json &value) {
    if (!value.contains("data")) {
      log::warn("api/kick", "No data object in Kick API get_channels response");
      return {};
    }

    std::vector<KickChannel> channels;
    nlohmann::json d = value["data"];

    for (const nlohmann::json &c : d) {
      channels.push_back(
          {c["broadcaster_user_id"], c["slug"], c["stream_title"],
           c["category"]["name"], c["stream"]["is_live"],
           utils::chrono::string_to_time_point(c["stream"]["start_time"],
                                               "%Y-%m-%dT%H:%M:%SZ")});
    }

    return channels;
  }

  std::vector<KickChannel> KickAPIClient::get_channels(
      const std::vector<int> &ids) const {
    if (this->authorization_key.empty()) {
      log::error("api/kick", "You must be authorized before using Kick API");
      return {};
    }

    if (ids.empty()) {
      return {};
    }

    std::string query;
    for (int i = 0; i < ids.size(); i++) {
      query += "broadcaster_user_id=" + std::to_string(ids.at(i));
      if (i + 1 < ids.size()) {
        query += "&";
      }
    }

    cpr::Response r = cpr::Get(
        cpr::Url{this->base_url + "/public/v1/channels?" + query},
        cpr::Header{{"Authorization", "Bearer " + this->authorization_key}});

    if (r.status_code != 200) {
      log::error("api/kick", "Failed to get Kick channels. Status code: " +
                                 std::to_string(r.status_code));
      return {};
    }

    nlohmann::json j = nlohmann::json::parse(r.text);
    return parse_json_channels(j);
  }

  std::vector<KickChannel> KickAPIClient::get_channels(
      const std::vector<std::string> &slugs) const {
    if (this->authorization_key.empty()) {
      log::error("api/kick", "You must be authorized before using Kick API");
      return {};
    }

    if (slugs.empty()) {
      return {};
    }

    std::string query;
    for (int i = 0; i < slugs.size(); i++) {
      query += "slug=" + slugs.at(i);
      if (i + 1 < slugs.size()) {
        query += "&";
      }
    }

    cpr::Response r = cpr::Get(
        cpr::Url{this->base_url + "/public/v1/channels?" + query},
        cpr::Header{{"Authorization", "Bearer " + this->authorization_key}});

    if (r.status_code != 200) {
      log::error("api/kick", "Failed to get Kick channels. Status code: " +
                                 std::to_string(r.status_code));
      return {};
    }

    nlohmann::json j = nlohmann::json::parse(r.text);
    return parse_json_channels(j);
  }

  void KickAPIClient::authorize() {
    cpr::Response r = cpr::Post(
        cpr::Url{"https://id.kick.com/oauth/"
                 "token?grant_type=client_credentials&client_id=" +
                 this->client_id + "&client_secret=" + this->client_secret});

    if (r.status_code != 200) {
      throw std::runtime_error(
          "Failed to authorize in Kick API. Status code: " +
          std::to_string(r.status_code));
    }

    nlohmann::json j = nlohmann::json::parse(r.text);

    this->authorization_key = j["access_token"];
    this->expires_in = j["expires_in"];
    this->token_acquired_timestamp = std::time(nullptr);

    log::info("api/kick", "Successfully authorized in Kick API!");
  }

  void KickAPIClient::refresh_token_thread() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(60));
      if (this->client_id.empty() || this->client_secret.empty()) {
        break;
      } else if (std::time(nullptr) - this->token_acquired_timestamp <
                 this->expires_in - 300) {
        continue;
      }

      log::info("api/kick", "Kick token is going to expire. Refreshing...");
      this->authorize();
    }
  }
}