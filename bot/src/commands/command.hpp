#pragma once

#include <memory>
#include <optional>
#include <sol/sol.hpp>
#include <sol/state.hpp>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "request.hpp"
#include "response.hpp"

namespace bot {
  namespace command {
    enum CommandArgument {
      SUBCOMMAND,
      MESSAGE,
      INTERVAL,
      NAME,
      TARGET,
      VALUE,
      AMOUNT,
    };

    class Command {
      public:
        virtual std::string get_name() const = 0;
        virtual Response run(const InstanceBundle &bundle,
                             const Request &request) const = 0;
        virtual schemas::PermissionLevel get_permission_level() const {
          return schemas::PermissionLevel::USER;
        }
        virtual int get_delay_seconds() const { return 5; }
        virtual std::vector<std::string> get_subcommand_ids() const {
          return {};
        }
    };

    class CommandLoader {
      public:
        CommandLoader();
        ~CommandLoader() = default;

        void add_command(std::unique_ptr<Command> cmd);
        void load_lua_directory(const std::string &folder_path);
        void load_lua_file(const std::string &file_path);
        std::optional<Response> run(const InstanceBundle &bundle,
                                    const Request &msg);

        const std::vector<std::unique_ptr<Command>> &get_commands() const {
          return this->commands;
        };

      private:
        std::vector<std::unique_ptr<Command>> commands;

        std::shared_ptr<sol::state> luaState;
    };
  }
}
