#include "command.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <sol/state.hpp>
#include <sol/types.hpp>
#include <stdexcept>
#include <string>

#include "../bundle.hpp"
#include "../modules/chatters.hpp"
#include "../modules/custom_command.hpp"
#include "../modules/event.hpp"
#include "../modules/help.hpp"
#include "../modules/join.hpp"
#include "../modules/massping.hpp"
#include "../modules/mcsrv.hpp"
#include "../modules/notify.hpp"
#include "../modules/settings.hpp"
#include "../modules/spam.hpp"
#include "../modules/timer.hpp"
#include "../modules/user.hpp"
#include "../utils/chrono.hpp"
#include "commands/lua.hpp"
#include "logger.hpp"
#include "request.hpp"
#include "response.hpp"

namespace bot {
  namespace command {
    CommandLoader::CommandLoader() {
      this->add_command(std::make_unique<mod::Massping>());
      this->add_command(std::make_unique<mod::Event>());
      this->add_command(std::make_unique<mod::Notify>());
      this->add_command(std::make_unique<mod::Join>());
      this->add_command(std::make_unique<mod::CustomCommand>());
      this->add_command(std::make_unique<mod::Timer>());
      this->add_command(std::make_unique<mod::Help>());
      this->add_command(std::make_unique<mod::Chatters>());
      this->add_command(std::make_unique<mod::Spam>());
      this->add_command(std::make_unique<mod::Settings>());
      this->add_command(std::make_unique<mod::User>());
      this->add_command(std::make_unique<mod::MinecraftServerCheck>());

      this->luaState = std::make_shared<sol::state>();
      this->luaState->open_libraries(sol::lib::base, sol::lib::string,
                                     sol::lib::math);

      lua::library::add_bot_library(this->luaState);
      lua::library::add_time_library(this->luaState);
    }

    void CommandLoader::load_lua_directory(const std::string &folder_path) {
      for (const auto &entry :
           std::filesystem::directory_iterator(folder_path)) {
        std::ifstream ifs(entry.path());
        if (!ifs.is_open()) {
          throw new std::runtime_error("Failed to open the Lua file at " +
                                       entry.path().string());
        }
        std::string content, line;

        while (std::getline(ifs, line)) {
          content += line + '\n';
        }

        ifs.close();

        this->add_command(
            std::make_unique<lua::LuaCommand>(this->luaState, content));
      }
    }

    void CommandLoader::add_command(std::unique_ptr<Command> command) {
      this->commands.push_back(std::move(command));
    }

    std::optional<Response> CommandLoader::run(const InstanceBundle &bundle,
                                               const Request &request) const {
      auto command = std::find_if(
          this->commands.begin(), this->commands.end(),
          [&](const auto &x) { return x->get_name() == request.command_id; });

      if (command == this->commands.end()) {
        return std::nullopt;
      }

      if ((*command)->get_permission_level() >
          request.user_rights.get_level()) {
        return std::nullopt;
      }

      pqxx::work work(request.conn);

      pqxx::result action_query = work.exec(
          "SELECT sent_at FROM actions WHERE user_id = " +
          std::to_string(request.user.get_id()) +
          " AND channel_id = " + std::to_string(request.channel.get_id()) +
          " AND command = '" + request.command_id + "' ORDER BY sent_at DESC");

      if (!action_query.empty()) {
        auto last_sent_at = utils::chrono::string_to_time_point(
            action_query[0][0].as<std::string>());

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

      std::string arguments;

      if (request.subcommand_id.has_value()) {
        arguments += request.subcommand_id.value() + " ";
      }

      if (request.message.has_value()) {
        arguments += request.message.value();
      }

      work.exec(
          "INSERT INTO actions(user_id, channel_id, command, arguments, "
          "full_message) VALUES (" +
          std::to_string(request.user.get_id()) + ", " +
          std::to_string(request.channel.get_id()) + ", '" +
          request.command_id + "', '" + arguments + "', '" +
          request.irc_message.message + "')");

      work.commit();

      return (*command)->run(bundle, request);
    }
  }
}
