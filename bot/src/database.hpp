#pragma once

#ifdef USE_POSTGRES
#include <pqxx/pqxx>
#elif defined(USE_MARIADB)
#include <mysql/mysql.h>
#endif

#include <cstring>
#include <map>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"

namespace bot::db {
  using DatabaseRow = std::map<std::string, std::string>;
  using DatabaseRows = std::vector<DatabaseRow>;

  struct BaseDatabase {
    public:
      virtual ~BaseDatabase() = default;

      template <typename T>
      std::vector<T> query_all(const std::string &query) {
        return this->query_all<T>(query, {});
      }

      template <typename T>
      std::vector<T> query_all(const std::string &query,
                               const std::vector<std::string> &params) {
        std::vector<T> results;

        for (DatabaseRow &row : this->exec(query, params)) {
          results.push_back(T(row));
        }

        return results;
      }

      virtual DatabaseRows exec(const std::string &sql) = 0;

      virtual DatabaseRows exec(const std::string &sql,
                                const std::vector<std::string> &parameters) = 0;

      virtual void close() = 0;
  };

#ifdef USE_POSTGRES
  struct PostgresDatabase : public BaseDatabase {
    public:
      pqxx::connection conn;

      PostgresDatabase(const std::string &credentials) : conn(credentials) {}

      DatabaseRows exec(const std::string &sql) override {
        pqxx::work work(conn);
        pqxx::result r = work.exec(sql);
        work.commit();

        std::vector<std::map<std::string, std::string>> rows;
        for (auto const &row : r) {
          std::map<std::string, std::string> m;
          for (auto const &f : row) {
            m[f.name()] = f.c_str() ? f.c_str() : "";
          }
          rows.push_back(m);
        }
        return rows;
      }

      DatabaseRows exec(const std::string &sql,
                        const std::vector<std::string> &parameters) override {
        pqxx::work work(conn);
        pqxx::result r = work.exec(sql, parameters);
        work.commit();

        std::vector<std::map<std::string, std::string>> rows;
        for (auto const &row : r) {
          std::map<std::string, std::string> m;
          for (auto const &f : row) {
            m[f.name()] = f.c_str() ? f.c_str() : "";
          }
          rows.push_back(m);
        }
        return rows;
      }

      void close() override { conn.close(); }
  };
#endif

#ifdef USE_MARIADB
  struct MariaDatabase : public BaseDatabase {
    public:
      MYSQL *conn = nullptr;

      MariaDatabase(const Configuration &cfg) : conn(mysql_init(nullptr)) {
        if (conn == nullptr) {
          throw std::runtime_error("mysql_init() failed");
        }

        if (!mysql_real_connect(
                conn, cfg.database.host.c_str(), cfg.database.user.c_str(),
                cfg.database.password.c_str(), cfg.database.name.c_str(),
                std::stoi(cfg.database.port), nullptr, 0)) {
          mysql_close(conn);
          throw std::runtime_error("mysql_real_connect() failed");
        }
      }

      ~MariaDatabase() { this->close(); }

      DatabaseRows exec(const std::string &sql) override {
        std::regex regex(R"(\$[0-9]+)");
        std::string query = std::regex_replace(sql, regex, "?");

        if (mysql_query(conn, query.c_str())) {
          std::string err = std::string(mysql_error(conn));
          mysql_close(conn);
          throw std::runtime_error("Query has failed. Error: " + err);
        }

        MYSQL_RES *res = mysql_store_result(conn);
        if (res == nullptr) {
          std::string err = std::string(mysql_error(conn));
          mysql_close(conn);
          throw std::runtime_error("mysql_store_result() has failed. Error: " +
                                   err);
        }

        int num_fields = mysql_num_fields(res);
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        MYSQL_ROW row;

        std::vector<std::map<std::string, std::string>> rows;

        while ((row = mysql_fetch_row(res))) {
          std::map<std::string, std::string> m;

          for (int i = 0; i < num_fields; i++) {
            m[fields[i].name] = row[i] == nullptr ? "" : row[i];
          }

          rows.push_back(std::move(m));
        }

        mysql_free_result(res);

        return rows;
      }

      DatabaseRows exec(const std::string &sql,
                        const std::vector<std::string> &parameters) override {
        std::regex regex(R"(\$[0-9]+)");
        std::string query = std::regex_replace(sql, regex, "?");

        MYSQL_STMT *stmt = mysql_stmt_init(conn);

        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
          std::string err = std::string(mysql_error(conn));
          mysql_stmt_close(stmt);
          throw std::runtime_error("Prepared query has failed. Error: " + err);
        }

        // binding input params
        std::vector<MYSQL_BIND> bind_params(parameters.size());
        std::vector<unsigned long> lengths(parameters.size());
        for (int i = 0; i < parameters.size(); i++) {
          memset(&bind_params[i], 0, sizeof(MYSQL_BIND));
          lengths[i] = parameters[i].size();
          bind_params[i].buffer_type = MYSQL_TYPE_STRING;
          bind_params[i].buffer = (void *)parameters[i].c_str();
          bind_params[i].buffer_length = lengths[i];
          bind_params[i].length = &lengths[i];
          bind_params[i].is_null = 0;
        }

        if (!parameters.empty() &&
            mysql_stmt_bind_param(stmt, bind_params.data())) {
          std::string err = std::string(mysql_error(conn));
          mysql_stmt_close(stmt);
          throw std::runtime_error(
              "mysql_stmt_bind_param() has failed. Error: " + err);
        }

        if (mysql_stmt_execute(stmt)) {
          std::string err = std::string(mysql_error(conn));
          mysql_stmt_close(stmt);
          throw std::runtime_error(
              "Prepared query execution has failed. Error: " + err);
        }

        // metadata
        MYSQL_RES *meta = mysql_stmt_result_metadata(stmt);
        if (!meta) {
          mysql_stmt_close(stmt);
          return {};
        }

        int num_fields = mysql_num_fields(meta);
        MYSQL_FIELD *fields = mysql_fetch_fields(meta);

        // bind output
        std::vector<MYSQL_BIND> bind_res(num_fields);
        std::vector<std::string> bufs(num_fields);
        std::vector<unsigned long> lengths_out(num_fields);
        std::vector<my_bool> is_null(num_fields);

        for (int i = 0; i < num_fields; i++) {
          bufs[i].resize(1024);
          memset(&bind_res[i], 0, sizeof(MYSQL_BIND));
          bind_res[i].buffer_type = MYSQL_TYPE_STRING;
          bind_res[i].buffer = bufs[i].data();
          bind_res[i].buffer_length = bufs[i].size();
          bind_res[i].length = &lengths_out[i];
          bind_res[i].is_null = &is_null[i];
        }

        if (mysql_stmt_bind_result(stmt, bind_res.data())) {
          std::string err = std::string(mysql_error(conn));
          mysql_free_result(meta);
          mysql_stmt_close(stmt);
          throw std::runtime_error(
              "mysql_stmt_bind_result() has failed. Error: " + err);
        }

        std::vector<std::map<std::string, std::string>> rows;

        while (mysql_stmt_fetch(stmt) == 0) {
          std::map<std::string, std::string> m;

          for (int i = 0; i < num_fields; i++) {
            m[fields[i].name] =
                bufs[i].data() == nullptr
                    ? ""
                    : std::string(bufs[i].data(), *bind_res[i].length);
          }

          rows.push_back(std::move(m));
        }

        mysql_free_result(meta);
        mysql_stmt_close(stmt);

        return rows;
      }

      void close() override {
        if (!conn) return;

        mysql_close(conn);
        conn = nullptr;
      }
  };
#endif

  std::unique_ptr<BaseDatabase> create_connection(const Configuration &cfg);
}