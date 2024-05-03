#include "stream.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace bot::stream {
  void StreamListenerClient::listen_channel(const int &id) {
    this->ids.push_back(id);
  }
  void StreamListenerClient::unlisten_channel(const int &id) {
    auto x = std::find_if(this->ids.begin(), this->ids.end(),
                          [&](const auto &x) { return x == id; });

    if (x != this->ids.end()) {
      this->ids.erase(x);
    }

    auto y = std::find_if(this->online_ids.begin(), this->online_ids.end(),
                          [&](const auto &x) { return x == id; });

    if (y != this->online_ids.end()) {
      this->online_ids.erase(y);
    }
  }
  void StreamListenerClient::run_thread() {
    std::thread t(&bot::stream::StreamListenerClient::run, this);
    t.join();
  }
  void StreamListenerClient::run() {
    while (true) {
      this->check();
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
  void StreamListenerClient::check() {
    auto streams = this->helix_client.get_streams(this->ids);

    // adding new ids
    for (const auto &stream : streams) {
      bool is_already_live =
          std::any_of(this->online_ids.begin(), this->online_ids.end(),
                      [&](const auto &x) { return x == stream.user_id; });

      if (!is_already_live) {
        this->online_ids.insert(stream.user_id);
      }
    }

    // removing old ids
    for (auto i = this->online_ids.begin(); i != this->online_ids.end(); i++) {
      bool is_live =
          std::any_of(streams.begin(), streams.end(),
                      [&](const auto &x) { return x.user_id == *i; });

      if (!is_live) {
        this->online_ids.erase(i);
      }
    }
  }
}
