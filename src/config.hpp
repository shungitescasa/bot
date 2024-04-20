#pragma once

#include <optional>
#include <string>

namespace bot {
  struct Configuration {
      std::string bot_username;
      std::string bot_password;
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
