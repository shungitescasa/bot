#pragma once

#include <string>

#include "core/command.hpp"
#include "core/log.hpp"

namespace scriptvm {
  class ScriptLoader : public bot::CommandLoader {
    public:
      ScriptLoader() : logger("ScriptLoader") {};
      ~ScriptLoader() = default;

      void load_directory(const std::string &path);
      void load_from_file(const std::string &path);
      virtual void add_from_string(const std::string &script) = 0;

    private:
      bot::Logger logger;
  };
}