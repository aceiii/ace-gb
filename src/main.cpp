#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

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

  CPU cpu;
  cpu.mmu = std::make_unique<MMU>();
  cpu.init();

  std::ifstream input("./boot.bin", std::ios::binary);
  if (input.fail()) {
    spdlog::error("Failed to load boot rom: {}", strerror(errno));
    return 1;
  }

  input.seekg(0, std::ios::end);
  auto boot_rom_size = input.tellg();
  input.seekg(0, std::ios::beg);

  std::vector<uint8_t> rom_bytes;
  rom_bytes.reserve(boot_rom_size);
  rom_bytes.insert(rom_bytes.begin(), std::istream_iterator<uint8_t>(input), std::istream_iterator<uint8_t>());

  cpu.mmu->load_boot_rom(rom_bytes.data());

  for (int i = 0; i < rom_bytes.size(); i++) {
    uint8_t byte = rom_bytes[i];
    cpu.mmu->write(i, byte);
  }

  spdlog::info("Starting CPU.");

  const auto fps = 60;
  const auto cycles_per_frame = kClockSpeed / fps;

  int cycles = 0;

  while (!cpu.state.halt) {
    do {
      cycles += cpu.execute();
    } while (cycles < cycles_per_frame);

    cycles -= cycles_per_frame;

    spdlog::info("{}", cpu.regs);
  }

  spdlog::info("Exiting.");

  return 0;
}
