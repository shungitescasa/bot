#include "github.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <pqxx/pqxx>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "irc/client.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "pqxx/internal/statement_parameters.hxx"
#include "schemas/channel.hpp"
#include "utils/string.hpp"

namespace bot {
  void GithubListener::run() {
    if (this->configuration.tokens.github_token->empty()) {
      log::warn("Github Listener",
                "Github token (token.github) must be set if you want to listen "
                "for changes in repositories.");
      return;
    }

    log::info("Github Listener", "Listening for new commits...");

    while (true) {
      this->check_for_listeners();

      std::unordered_map<std::string, std::vector<Commit>> new_commits =
          this->check_new_commits();

      this->notify_about_commits(new_commits);

      for (const auto &pair : new_commits) {
        std::vector<std::string> &commits = this->commits.at(pair.first);
        auto &s = pair.second;
        commits.reserve(commits.size() + std::distance(s.begin(), s.end()));

        std::vector<std::string> commit_shas;
        std::transform(s.begin(), s.end(), std::back_inserter(commit_shas),
                       [](const Commit &x) { return x.sha; });

        // i could do it with .insert(), but im tired of these errors
        for (std::string &sha : commit_shas) {
          commits.push_back(sha);
        }
      }

      std::this_thread::sleep_for(std::chrono::seconds(60));
    }
  }

  void GithubListener::check_for_listeners() {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result repos =
        work.exec("SELECT custom_alias_id FROM events WHERE event_type = 10");

    // Adding new repos
    for (const auto &repo : repos) {
      std::string id = repo[0].as<std::string>();
      if (std::any_of(this->ids.begin(), this->ids.end(),
                      [&id](const auto &x) { return x == id; }))
        continue;

      this->ids.push_back(id);
      this->commits.insert({id, {}});
    }

    // Deleting old repos
    std::vector<std::string> names_to_delete;

    for (const std::string &id : this->ids) {
      if (std::any_of(repos.begin(), repos.end(), [&id](const pqxx::row &x) {
            return x[0].as<std::string>() == id;
          }))
        continue;

      names_to_delete.push_back(id);
    }

    for (const std::string &name : names_to_delete) {
      auto id_pos = std::find(this->ids.begin(), this->ids.end(), name);
      this->ids.erase(id_pos);
      this->commits.erase(name);
    }

    work.commit();
    conn.close();
  }

  std::unordered_map<std::string, std::vector<Commit>>
  GithubListener::check_new_commits() {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    pqxx::result repos =
        work.exec("SELECT custom_alias_id FROM events WHERE event_type = 10");

    std::unordered_map<std::string, std::vector<Commit>> new_commits;

    for (const std::string &id : this->ids) {
      cpr::Response response = cpr::Get(
          cpr::Url{"https://api.github.com/repos/" + id + "/commits"},
          cpr::Header{
              {"Authorization",
               "Bearer " + this->configuration.tokens.github_token.value()}},
          cpr::Header{{"Accept", "application/vnd.github+json"},
                      {"X-GitHub-Api-Version", "2022-11-28"},
                      {"User-Agent", "https://github.com/ilotterytea/bot"}});

      if (response.status_code != 200) {
        log::error("Github Listener", "Got HTTP status " +
                                          std::to_string(response.status_code) +
                                          " for " + id);
        continue;
      }

      nlohmann::json j = nlohmann::json::parse(response.text);
      const std::vector<std::string> &commit_cache = this->commits.at(id);
      std::vector<Commit> repo_commits;

      for (const auto &commit : j) {
        const std::string &sha = commit["sha"];

        if (std::any_of(commit_cache.begin(), commit_cache.end(),
                        [&sha](const std::string &x) { return x == sha; }))
          continue;

        const std::string &commiter_name = commit["committer"]["login"];
        const std::string &message = commit["commit"]["message"];

        repo_commits.push_back({sha, commiter_name, message});
      }

      new_commits.insert({id, repo_commits});
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    work.commit();
    conn.close();

    return new_commits;
  }

  void GithubListener::notify_about_commits(
      const std::unordered_map<std::string, std::vector<Commit>> &new_commits) {
    pqxx::connection conn(GET_DATABASE_CONNECTION_URL(this->configuration));
    pqxx::work work(conn);

    for (const auto &pair : new_commits) {
      // don't notify on startup
      if (this->commits.at(pair.first).size() == 0) continue;

      pqxx::result events = work.exec(
          "SELECT id, channel_id, message, flags FROM events WHERE "
          "custom_alias_id "
          "= '" +
          pair.first + "' AND event_type = 10");

      for (const auto &event : events) {
        schemas::Channel channel(
            work.exec("SELECT * FROM channels WHERE id = " +
                      std::to_string(event[1].as<int>()))[0]);

        pqxx::result subscriber_ids = work.exec(
            "SELECT user_id FROM event_subscriptions WHERE event_id = " +
            std::to_string(event[0].as<int>()));

        std::vector<std::string> subscriber_names;

        for (const auto &subscriber_id : subscriber_ids) {
          pqxx::result subscriber_name =
              work.exec("SELECT alias_name FROM users WHERE id = " +
                        std::to_string(subscriber_id[0].as<int>()));
          subscriber_names.push_back(subscriber_name[0][0].as<std::string>());
        }

        // TODO: implement massping flag

        for (const Commit &commit : pair.second) {
          std::string message = event[2].as<std::string>();

          // Replacing SHA placeholder
          std::size_t pos = message.find("%0");
          if (pos != std::string::npos)
            message.replace(pos, 2, commit.sha.substr(0, 7));

          // Replacing committer placeholder
          pos = message.find("%1");
          if (pos != std::string::npos)
            message.replace(pos, 2, commit.commiter_name);

          // Replacing message placeholder
          pos = message.find("%2");
          if (pos != std::string::npos) message.replace(pos, 2, commit.message);

          std::vector<std::vector<std::string>> ping_names =
              utils::string::separate_by_length(subscriber_names,
                                                500 - message.length());

          if (ping_names.empty()) {
            this->irc_client.say(channel.get_alias_name(), message);
          } else {
            for (const std::vector<std::string> &ping_names_vec : ping_names) {
              std::string pings = utils::string::str(ping_names_vec.begin(),
                                                     ping_names_vec.end(), ' ');

              this->irc_client.say(channel.get_alias_name(),
                                   message + " · " + pings);
            }
          }
        }
      }
    }

    work.commit();
    conn.close();
  }
}