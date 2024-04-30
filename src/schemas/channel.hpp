#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <pqxx/pqxx>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bot::schemas {
  class Channel {
    public:
      Channel(const pqxx::row &row) {
        this->id = row[0].as<int>();
        this->alias_id = row[1].as<int>();
        this->alias_name = row[2].as<std::string>();

        std::tm tm = {};
        std::stringstream ss(row[3].as<std::string>());

        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) {
          throw std::invalid_argument("Invalid time format");
        }

        this->joined_at =
            std::chrono::system_clock::from_time_t(std::mktime(&tm));

        if (!row[4].is_null()) {
          tm = {};
          ss = std::stringstream(row[4].as<std::string>());

          ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

          if (ss.fail()) {
            throw std::invalid_argument("Invalid time format");
          }

          this->opted_out_at =
              std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
      }

      ~Channel() = default;

      const int &get_id() const { return this->id; }
      const int &get_alias_id() const { return this->alias_id; }
      const std::string &get_alias_name() const { return this->alias_name; }
      const std::chrono::system_clock::time_point &get_joined_at() const {
        return this->joined_at;
      }
      const std::optional<std::chrono::system_clock::time_point> &
      get_opted_out_at() const {
        return this->opted_out_at;
      }

    private:
      int id, alias_id;
      std::string alias_name;
      std::chrono::system_clock::time_point joined_at;
      std::optional<std::chrono::system_clock::time_point> opted_out_at;
  };
}
