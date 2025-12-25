#include "command.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <optional>
#include <sol/state.hpp>
#include <sol/types.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "../bundle.hpp"
#include "../utils/chrono.hpp"
#include "commands/lua.hpp"
#include "database.hpp"
#include "request.hpp"
#include "response.hpp"
#include "schemas/user.hpp"
#include "utils/string.hpp"

namespace bot {
  namespace command {
    CommandLoader::CommandLoader() {
      this->add_command(std::make_unique<lua::mod::LuaExecution>());
      this->add_command(std::make_unique<lua::mod::LuaRemoteExecution>());

      this->luaState = std::make_shared<sol::state>();
      this->luaState->open_libraries(sol::lib::base, sol::lib::string,
                                     sol::lib::table, sol::lib::math);

      lua::library::add_base_libraries(this->luaState);
    }

    void CommandLoader::load_lua_directory(const std::string &folder_path) {
      for (const auto &entry :
           std::filesystem::directory_iterator(folder_path)) {
        load_lua_file(entry.path());
      }
    }

    void CommandLoader::load_lua_file(const std::string &file_path) {
      std::ifstream ifs(file_path);
      if (!ifs.is_open()) {
        throw new std::runtime_error("Failed to open the Lua file at " +
                                     file_path);
      }
      std::string content, line;

      while (std::getline(ifs, line)) {
        content += line + '\n';
      }

      ifs.close();

      this->add_command(
          std::make_unique<lua::LuaCommand>(this->luaState, content));
    }

    void CommandLoader::add_command(std::unique_ptr<Command> command) {
      auto it = std::find_if(this->commands.begin(), this->commands.end(),
                             [&command](const auto &x) {
                               return command->get_name() == x->get_name();
                             });
      if (it != this->commands.end()) {
        this->commands.erase(it);
      }
      this->commands.push_back(std::move(command));
    }

    std::optional<Response> CommandLoader::run(const InstanceBundle &bundle,
                                               const Request &request) {
      lua::library::add_chat_libraries(this->luaState, request, bundle);

      auto command = std::find_if(
          this->commands.begin(), this->commands.end(),
          [&](const auto &x) { return x->get_name() == request.command_id; });

      if (command == this->commands.end()) {
        return std::nullopt;
      }

      if ((*command)->get_permission_level() >
          request.requester.user_rights.get_level()) {
        return std::nullopt;
      }

      std::unique_ptr<db::BaseDatabase> conn =
          db::create_connection(bundle.configuration);

      if (request.requester.user_rights.get_level() < schemas::SUPERUSER) {
        db::DatabaseRows actions = conn->exec(
            "SELECT sent_at FROM actions WHERE user_id = $1 AND channel_id = "
            "$2 "
            "AND command = $3 ORDER BY sent_at DESC",
            {std::to_string(request.requester.user.get_id()),
             std::to_string(request.requester.channel.get_id()),
             request.command_id});

        if (!actions.empty()) {
          auto last_sent_at =
              utils::chrono::string_to_time_point(actions[0]["sent_at"]);

          auto now = std::chrono::system_clock::now();
          auto now_time_it = std::chrono::system_clock::to_time_t(now);
          auto now_tm = std::gmtime(&now_time_it);
          now = std::chrono::system_clock::from_time_t(std::mktime(now_tm));

          auto difference = std::chrono::duration_cast<std::chrono::seconds>(
              now - last_sent_at);

          if (difference.count() < command->get()->get_delay_seconds()) {
            return std::nullopt;
          }
        }
      }

      std::string arguments;

      if (request.subcommand_id.has_value()) {
        arguments += request.subcommand_id.value() + " ";
      }

      if (request.message.has_value()) {
        arguments += request.message.value();
      }

      Response response = (*command)->run(bundle, request);

      std::string response_action;
      if (response.is_single()) {
        response_action = response.get_single();
      } else if (response.is_multiple()) {
        auto &v = response.get_multiple();
        response_action = utils::string::str(v.begin(), v.end(), '\n');
      }

      conn->exec(
          "INSERT INTO actions(user_id, channel_id, command, arguments, "
          "response) VALUES ($1, $2, $3, $4, $5)",
          {std::to_string(request.requester.user.get_id()),
           std::to_string(request.requester.channel.get_id()),
           request.command_id, arguments, response_action});

      return response;
    }
  }
}
