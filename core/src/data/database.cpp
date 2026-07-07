#include "core/data/database.hpp"

#if USE_POSTGRES
#include <format>
#endif
#include <memory>

namespace bot::data {
  std::unique_ptr<BaseDatabase> create_connection(const Configuration &cfg) {
#if USE_POSTGRES
    return std::make_unique<PostgresDatabase>(
        std::format("dbname = {} user = {} password = {} host = {} port = {}",
                    cfg.database.name, cfg.database.user, cfg.database.password,
                    cfg.database.host, cfg.database.port));
#elif defined(USE_MARIADB)
    return std::make_unique<MariaDatabase>(cfg);
#endif
  }

  std::unique_ptr<BaseDatabase> create_connection() {
    Configuration &cfg = Configuration::get_instance();
    return create_connection(cfg);
  }
}