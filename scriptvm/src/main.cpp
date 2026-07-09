#include <chrono>
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

const std::chrono::time_point<std::chrono::steady_clock> START_TIME =
    std::chrono::steady_clock::now();

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

  server.bind("alive", []() { return true; });
  server.bind("uptime", []() {
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - START_TIME)
            .count());
  });

  server.bind("exec",
              [&](bot::Request request) { return loader->run(request); });

  server.bind("list", [&]() {
    bot::CommandDataVec list;
    list.reserve(loader->get_commands().size());

    for (const bot::CommandBox &c : loader->get_commands())
      list.push_back(c->data());

    return list;
  });

  server.run();

  return 0;
}