#pragma once

// sol fails to find lua on itself so we have to include it right here
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <memory>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <string>

#include "core/command.hpp"
#include "scriptvm/script.hpp"

namespace scriptvm::lua {
  bot::Response run_safe_lua_script(const bot::Request &request,
                                    const std::string &script,
                                    std::string lua_id, bool moon_prefix);

  class LuaCommand : public bot::Command {
    public:
      LuaCommand(std::shared_ptr<sol::state> state,
                 const std::string &contents);
      ~LuaCommand() = default;

      const bot::Response run(const bot::Request &request) const override;

    private:
      sol::function handle;
      std::shared_ptr<sol::state> state;
  };

  class LuaScriptLoader : public ScriptLoader {
    public:
      LuaScriptLoader();
      ~LuaScriptLoader() = default;

      void add_from_string(const std::string &script) override {
        this->add(std::make_shared<LuaCommand>(this->lua, script));
      }

      const bot::Response execute(const std::string &script,
                                  const bot::Request &request) override;

    private:
      std::shared_ptr<sol::state> lua;
  };

  namespace libraries {
    void open_base_libraries(std::shared_ptr<sol::state> state,
                             LuaScriptLoader *loader);
    void open_extended_libraries(std::shared_ptr<sol::state> state,
                                 LuaScriptLoader *loader);

    void open_storage_library(std::shared_ptr<sol::state> state,
                              const bot::Requester &request,
                              const std::string &lua_id);
  }
}
