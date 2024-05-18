#include "localization.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../utils/string.hpp"
#include "line_id.hpp"

namespace bot {
  namespace loc {
    Localization::Localization(const std::string &folder_path) {
      for (const auto &entry :
           std::filesystem::directory_iterator(folder_path)) {
        std::vector<std::string> file_name_parts =
            utils::string::split_text(entry.path(), '/');
        std::string file_name = file_name_parts[file_name_parts.size() - 1];
        file_name = file_name.substr(0, file_name.length() - 5);

        std::unordered_map<LineId, std::string> lines =
            this->load_from_file(entry.path());

        this->localizations[file_name] = lines;
      }
    }

    std::unordered_map<LineId, std::string> Localization::load_from_file(
        const std::string &file_path) {
      std::ifstream ifs(file_path);

      std::unordered_map<LineId, std::string> map;

      nlohmann::json json;
      ifs >> json;

      for (auto it = json.begin(); it != json.end(); ++it) {
        std::optional<LineId> line_id = string_to_line_id(it.key());

        if (line_id.has_value()) {
          map[line_id.value()] = it.value();
        }
      }

      ifs.close();
      return map;
    }

    std::optional<std::string> Localization::get_localized_line(
        const std::string &locale_id, const LineId &line_id) const {
      auto locale_it =
          std::find_if(this->localizations.begin(), this->localizations.end(),
                       [&](const auto &x) { return x.first == locale_id; });

      if (locale_it == this->localizations.end()) {
        return std::nullopt;
      }

      auto line_it =
          std::find_if(locale_it->second.begin(), locale_it->second.end(),
                       [&](const auto &x) { return x.first == line_id; });

      if (line_it == locale_it->second.end()) {
        return std::nullopt;
      }

      return line_it->second;
    }

    std::optional<std::string> Localization::get_formatted_line(
        const std::string &locale_id, const LineId &line_id,
        const std::vector<std::string> &args) const {
      std::optional<std::string> o_line =
          this->get_localized_line(locale_id, line_id);

      if (!o_line.has_value()) {
        return std::nullopt;
      }

      std::string line = o_line.value();

      int pos = 0;
      int index = 0;

      while ((pos = line.find("%s", pos)) != std::string::npos) {
        line.replace(pos, 2, args[index]);
        pos += args[index].size();
        ++index;

        if (index >= args.size()) {
          break;
        }
      }

      return line;
    }

    std::optional<std::string> Localization::get_formatted_line(
        const command::Request &request, const LineId &line_id,
        const std::vector<std::string> &args) const {
      std::optional<std::string> o_line = this->get_formatted_line(
          request.channel_preferences.get_locale(), line_id, args);

      if (!o_line.has_value()) {
        return std::nullopt;
      }

      std::string line = o_line.value();

      std::map<std::string, std::string> token_map = {
          {"{sender.alias_name}", request.user.get_alias_name()},
          {"{source.alias_name}", request.channel.get_alias_name()},
          {"{default.prefix}", DEFAULT_PREFIX}};

      for (const auto &pair : token_map) {
        int pos = line.find(pair.first);

        while (pos != std::string::npos) {
          line.replace(pos, pair.first.length(), pair.second);
          pos = line.find(pair.first, pos + pair.second.length());
        }
      }

      return line;
    }
  }
}
