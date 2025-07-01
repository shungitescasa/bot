#include "database.hpp"

#include <memory>

namespace bot::db {
  std::unique_ptr<BaseDatabase> create_connection(const Configuration &cfg) {
#if USE_POSTGRES
    return std::make_unique<PostgresDatabase>(GET_DATABASE_CONNECTION_URL(cfg));
#elif defined(USE_MARIADB)
    return std::make_unique<MariaDatabase>(cfg);
#endif
  }
}