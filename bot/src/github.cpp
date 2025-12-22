#include "github.hpp"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "database.hpp"
#include "irc/client.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "schemas/stream.hpp"
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
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(this->configuration);

    db::DatabaseRows repos =
        conn->exec("SELECT name FROM events WHERE event_type = 40");

    // Adding new repos
    for (const auto &repo : repos) {
      std::string id = repo.at("name");
      if (std::any_of(this->ids.begin(), this->ids.end(),
                      [&id](const auto &x) { return x == id; }))
        continue;

      this->ids.push_back(id);
      this->commits.insert({id, {}});
    }

    // Deleting old repos
    std::vector<std::string> names_to_delete;

    for (const std::string &id : this->ids) {
      if (std::any_of(
              repos.begin(), repos.end(),
              [&id](const db::DatabaseRow &x) { return x.at("name") == id; }))
        continue;

      names_to_delete.push_back(id);
    }

    for (const std::string &name : names_to_delete) {
      auto id_pos = std::find(this->ids.begin(), this->ids.end(), name);
      this->ids.erase(id_pos);
      this->commits.erase(name);
    }
  }

  std::unordered_map<std::string, std::vector<Commit>>
  GithubListener::check_new_commits() {
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

        const std::string &commiter_name = commit["author"]["login"];
        const std::string &message = commit["commit"]["message"];

        repo_commits.push_back({sha, commiter_name, message});
      }

      new_commits.insert({id, repo_commits});
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return new_commits;
  }

  void GithubListener::notify_about_commits(
      const std::unordered_map<std::string, std::vector<Commit>> &new_commits) {
    std::unique_ptr<db::BaseDatabase> conn =
        db::create_connection(this->configuration);

    for (const auto &pair : new_commits) {
      // don't notify on startup
      if (this->commits.at(pair.first).size() == 0) continue;

      db::DatabaseRows events = conn->exec(
          "SELECT e.id, e.message, is_massping, "
          "c.alias_name AS "
          "channel_name, c.alias_id AS channel_aid "
          "FROM events e "
          "INNER JOIN channels c ON c.id = e.channel_id "
          "WHERE e.name = $1 AND e.event_type = 40",
          {pair.first});

      for (const auto &event : events) {
        std::vector<std::string> names;

        bool massping_enabled = std::stoi(event.at("is_massping"));

        if (massping_enabled) {
          auto chatters = this->helix_client.get_chatters(
              std::stoi(event.at("channel_aid")),
              this->irc_client.get_user_id());

          std::for_each(chatters.begin(), chatters.end(),
                        [&names](const auto &u) { names.push_back(u.login); });
        } else {
          db::DatabaseRows subs = conn->exec(
              "SELECT u.alias_name FROM users u INNER JOIN event_subscriptions "
              "es ON es.user_id = u.id WHERE es.event_id = $1",
              {event.at("id")});

          std::for_each(subs.begin(), subs.end(),
                        [&names](const db::DatabaseRow &x) {
                          names.push_back(x.at("alias_name"));
                        });
        }

        for (const Commit &commit : pair.second) {
          std::string message = event.at("message");
          message = "🧑‍💻 " + message;

          // Replacing SHA placeholder
          std::size_t pos = message.find("{sha}");
          if (pos != std::string::npos)
            message.replace(pos, 5, commit.sha.substr(0, 7));

          // Replacing committer placeholder
          pos = message.find("{author}");
          if (pos != std::string::npos)
            message.replace(pos, 8, commit.commiter_name);

          // Replacing message placeholder
          pos = message.find("{msg}");
          if (pos != std::string::npos) message.replace(pos, 5, commit.message);

          if (!names.empty()) {
            message += " · ";
          }

          std::vector<std::string> parts =
              utils::string::separate_by_length(message, names, "@", " ", 500);

          std::for_each(parts.begin(), parts.end(),
                        [&message, &event, this](const std::string &part) {
                          this->irc_client.say(
                              std::stoi(event.at("channel_aid")),
                              message + part);
                        });
        }
      }
    }

    conn->close();
  }
}