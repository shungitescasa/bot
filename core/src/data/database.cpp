#include "core/data/database.hpp"

#if defined(USE_POSTGRES) || defined(USE_MARIADB)

#ifdef USE_POSTGRES
#include <format>
#endif

#include <memory>

namespace bot::data {
  DatabaseConnection create_connection(const Configuration &cfg) {
#ifdef USE_POSTGRES
    return std::make_unique<PostgresDatabase>(
        std::format("dbname = {} user = {} password = {} host = {} port = {}",
                    cfg.database.name, cfg.database.user, cfg.database.password,
                    cfg.database.host, cfg.database.port));
#elifdef USE_MARIADB
    return std::make_unique<MariaDatabase>(cfg);
#else
    return nullptr;
#endif
  }

  DatabaseConnection create_connection() {
    Configuration &cfg = Configuration::get_instance();
    return create_connection(cfg);
  }
}

#endif