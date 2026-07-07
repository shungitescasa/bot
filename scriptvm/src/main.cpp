#include <format>
#include <optional>

#include "core/config.hpp"
#include "core/log.hpp"
#include "rpc/server.h"

int main(int argc, char *argv[]) {
  bot::Logger logger("ScriptVM-Main");
  logger.info("Starting...");

  bot::Configuration &cfg = bot::Configuration::get_instance();
  cfg.load_from_args(argc, argv);

  logger.info(
      std::format("Running C++ RPC server on port {}...", cfg.rpc.port));

  rpc::server server(cfg.rpc.port);

  server.bind("execute_untrusted_script",
              [](std::string script) { return std::make_optional("ok"); });

  server.run();

  return 0;
}