#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "line_id.hpp"

namespace bot {
  namespace loc {
    class Localization {
      public:
        Localization(const std::string &folder_path);
        ~Localization() = default;

        std::optional<std::string> get_localized_line(
            const std::string &locale_id, const LineId &line_id);

        std::optional<std::string> get_formatted_line(
            const std::string &locale_id, const LineId &line_id,
            const std::vector<std::string> &args);

      private:
        std::unordered_map<LineId, std::string> load_from_file(
            const std::string &file_path);
        std::unordered_map<std::string, std::unordered_map<LineId, std::string>>
            localizations;
    };
  }

}
