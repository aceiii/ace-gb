#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "args.hpp"


using namespace app;

namespace {
const std::string kSettingsFileName = "settings.toml";
const std::string kDefaultLogLevel = "info";
}

static bool SetLoggingLevel(std::string_view level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

std::expected<Args, std::string> app::GetArgs(std::string_view name, std::string_view version, int argc, char** argv) {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program(std::string{name}, std::string{version});

  program.add_argument("--log-level")
      .help("Set the verbosity for logging")
      .default_value(kDefaultLogLevel)
      .nargs(1);

  program.add_argument("--settings")
    .help("Settings file to load/save")
    .default_value(kSettingsFileName)
    .nargs(1);

  program.add_argument("--doctor-log")
    .help("Outputs log file for gameboy-doctor")
    .implicit_value(true)
    .default_value(false);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& err) {
    std::stringstream ss;
    ss << err.what() << "\n";
    ss << program;
    return std::unexpected{ss.str()};
  }

  const std::string level = program.get("--log-level");
  if (!SetLoggingLevel(level)) {
    std::stringstream ss;
    ss << std::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}\n",
                             level);
    ss << program;
    return std::unexpected{ss.str()};
  }

  auto doctor_log = program.get<bool>("--doctor-log");
  if (doctor_log) {
    try {
      auto logger = spdlog::basic_logger_mt("doctor_logger", "doctor.log");
      logger->set_pattern("%v");
    }
    catch (const spdlog::spdlog_ex& ex) {
      return std::unexpected{std::format("Doctor logger failed to initialize: {}", ex.what())};
    }
  }

  return Args{
    .settings_filename = program.get<std::string>("--settings"),
    .log_level = level,
    .doctor_log = doctor_log,
  };
}
