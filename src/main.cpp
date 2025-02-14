#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "cpu.h"
#include "interface.h"
#include "registers.h"
#include "mmu.h"

using namespace std::chrono_literals;

static bool set_logging_level(const std::string &level_name) {
  auto level = magic_enum::enum_cast<spdlog::level::level_enum>(level_name);
  if (level.has_value()) {
    spdlog::set_level(level.value());
    return true;
  }
  return false;
}

auto main(int argc, char *argv[]) -> int {
  spdlog::set_level(spdlog::level::info);

  argparse::ArgumentParser program("ace-gb", "0.0.1");

  program.add_argument("--log-level")
      .help("Set the verbosity for logging")
      .default_value(std::string("info"))
      .nargs(1);

  program.add_argument("--doctor-log")
    .help("Outputs log file for gameboy-doctor")
    .implicit_value(true)
    .default_value(false);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  const std::string level = program.get("--log-level");
  if (!set_logging_level(level)) {
    std::cerr << fmt::format("Invalid argument \"{}\" - allowed options: "
                             "{{trace, debug, info, warn, err, critical, off}}",
                             level)
              << std::endl;
    std::cerr << program;
    return 1;
  }

  auto doctor_log = program.get<bool>("--doctor-log");
  if (doctor_log) {
    try {
      auto logger = spdlog::basic_logger_mt("doctor_logger", "doctor.log");
      logger->set_pattern("%v");
    }
    catch (const spdlog::spdlog_ex &ex) {
      std::cerr << "Doctor logger failed to initialize: " << ex.what() << std::endl;
      return 1;
    }
  }

  Interface interface;
  interface.run();

  return 0;
}
