#include "database.hpp"

#include <memory>

#include "config.hpp"

namespace bot::db {
  std::unique_ptr<BaseDatabase> create_connection(const Configuration &cfg) {
#if USE_POSTGRES
    return std::make_unique<PostgresDatabase>(GET_DATABASE_CONNECTION_URL(cfg));
#elif defined(USE_MARIADB)
    return std::make_unique<MariaDatabase>(cfg);
#endif
  }

  std::unique_ptr<BaseDatabase> create_connection() {
    Configuration &cfg = Configuration::get_instance();
    return create_connection(cfg);
  }
}