#include "command.hpp"

#include <algorithm>
#include <memory>
#include <optional>

#include "../bundle.hpp"
#include "../modules/ping.hpp"
#include "request.hpp"

namespace bot {
  namespace command {
    CommandLoader::CommandLoader() {
      this->add_command(std::make_unique<mod::Ping>());
    }

    void CommandLoader::add_command(std::unique_ptr<Command> command) {
      this->commands.push_back(std::move(command));
    }

    std::optional<std::variant<std::vector<std::string>, std::string>>
    CommandLoader::run(const InstanceBundle &bundle,
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

      return (*command)->run(bundle, request);
    }
  }
}
