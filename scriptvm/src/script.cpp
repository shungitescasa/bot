#include "scriptvm/script.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

namespace scriptvm {
  void ScriptLoader::load_directory(const std::string &path) {
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      this->load_from_file(entry.path());
    }
    logger.info(
        std::format("Loaded {} script commands", this->commands.size()));
  }

  void ScriptLoader::load_from_file(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      throw std::runtime_error("Failed to open script file: " + path);
    }

    logger.debug("Loading " + path + "...");

    std::string contents, line;
    while (std::getline(ifs, line)) contents += line + '\n';

    ifs.close();

    this->add_from_string(contents);
  }
}