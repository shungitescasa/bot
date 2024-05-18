#pragma once

#include <optional>
#include <string>
namespace botweb {
  struct Configuration {
      std::string contact_name = "some guy", contact_url = "#";
  };

  std::optional<Configuration> parse_configuration_from_file(
      const std::string &file_path);
}
