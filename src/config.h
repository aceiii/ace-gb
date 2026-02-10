#ifndef ACE_GB_CONFIG_H
#define ACE_GB_CONFIG_H

#include <expected>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>

template <typename Settings>
struct Config {
  Settings settings;

  std::function<void(const Settings &settings, toml::table&)> serialize;
  std::function<void(const toml::table&, Settings &settings)> deserialize;

  std::expected<void, std::string> Load(std::string_view filename) {
    auto result = toml::parse_file(filename);
    if (!result) {
      return std::unexpected(std::format("Failed to parse settings file: {}", result.error().description()));
    }

    try {
      deserialize(result.table(), settings);
    } catch (std::exception &e) {
      return std::unexpected{std::format("Failed to deserialize settings: {}", e.what())};
    }

    return {};
  }

  [[nodiscard]] std::expected<void, std::string> Save(std::string_view filename) const {
    toml::table table;
    try {
      serialize(settings, table);
    } catch (std::exception &e) {
      return std::unexpected{std::format("Failed to serialize settings: {}", e.what())};
    }

    try {
      namespace fs = std::filesystem;
      fs::path filepath{filename};
      std::ofstream file(filepath);
      file << toml::toml_formatter{ table };
      file.close();
    } catch (std::exception &e) {
      return std::unexpected{std::format("Failed to save settings to file: {}", e.what())};
    }
    return {};
  }
};


#endif //ACE_GB_CONFIG_H
