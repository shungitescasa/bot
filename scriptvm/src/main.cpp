#include <format>
#include <memory>
#include <optional>
#include <stdexcept>

#include "core/command.hpp"
#include "core/config.hpp"
#include "core/log.hpp"
#include "rpc/server.h"
#include "scriptvm/lua.hpp"
#include "scriptvm/script.hpp"

int main(int argc, char *argv[]) {
  bot::Logger logger("ScriptVM-Main");
  logger.info("Starting...");

  bot::Configuration &cfg = bot::Configuration::get_instance();
  cfg.load_from_args(argc, argv);

  // loading scripts
  logger.info("Loading scripts...");
  std::shared_ptr<scriptvm::ScriptLoader> loader;

  if (cfg.script.loader == "lua") {
    logger.info("Script loader: " + cfg.script.loader);
    loader = std::make_shared<scriptvm::lua::LuaScriptLoader>();
  } else {
    throw std::runtime_error("Unsupported script loader: " + cfg.script.loader);
  }

  loader->load_directory(cfg.script.directory);

  logger.info(
      std::format("Running C++ RPC server on port {}...", cfg.rpc.port));

  rpc::server server(cfg.rpc.port);

  server.bind("execute_untrusted_script",
              [](std::string script) { return std::make_optional("ok"); });

  server.bind("exec", [&](const bot::Request &request) {
    return loader->run(request);
  });

  server.run();

  return 0;
}