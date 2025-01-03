#include <argparse/argparse.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "cpu.h"
#include "interface.h"
#include "registers.h"

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

  CPU cpu(65536);
  Registers &regs = cpu.regs;

  regs.set(Reg8::A, 1);
  regs.set(Reg8::F, 0b10001111);
  regs.set(Reg8::B, 3);
  regs.set(Reg8::C, 4);
  regs.set(Reg8::D, 5);
  regs.set(Reg8::E, 6);
  regs.set(Reg8::H, 7);
  regs.set(Reg8::L, 8);
  regs.sp = 123;
  regs.pc = 45678;

  regs.set(Flag::Z, 1);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
  regs.set(Flag::C, 0);

  spdlog::info("flag z: {}", regs.get(Flag::Z));
  spdlog::info("flag n: {}", regs.get(Flag::N));
  spdlog::info("flag h: {}", regs.get(Flag::H));
  spdlog::info("flag c: {}", regs.get(Flag::C));

  spdlog::info("a: {:08b} = {}", regs.get(Reg8::A), regs.get(Reg8::A));
  spdlog::info("f: {:08b} = {}", regs.get(Reg8::F), regs.get(Reg8::F));
  spdlog::info("af: {:016b} = {}", regs.get(Reg16::AF), regs.get(Reg16::AF));

  spdlog::info("b: {:08b} = {}", regs.get(Reg8::B), regs.get(Reg8::B));
  spdlog::info("c: {:08b} = {}", regs.get(Reg8::C), regs.get(Reg8::C));
  spdlog::info("bc: {:016b} = {}", regs.get(Reg16::BC), regs.get(Reg16::BC));

  spdlog::info("d: {:08b} = {}", regs.get(Reg8::D), regs.get(Reg8::D));
  spdlog::info("e: {:08b} = {}", regs.get(Reg8::E), regs.get(Reg8::E));
  spdlog::info("de: {:016b} = {}", regs.get(Reg16::DE), regs.get(Reg16::DE));

  spdlog::info("h: {:08b} = {}", regs.get(Reg8::H), regs.get(Reg8::H));
  spdlog::info("l: {:08b} = {}", regs.get(Reg8::L), regs.get(Reg8::L));
  spdlog::info("hl: {:016b} = {}", regs.get(Reg16::HL), regs.get(Reg16::HL));

  spdlog::info("sp: {:08b} = {}", regs.sp, regs.sp);
  spdlog::info("pc: {:08b} = {}", regs.pc, regs.pc);

  regs.set(Reg16::AF, 4567);
  regs.set(Reg16::BC, 6678);
  regs.set(Reg16::DE, 1193);
  regs.set(Reg16::HL, 2356);

  spdlog::info("af: {:016b} = {}", regs.get(Reg16::AF), regs.get(Reg16::AF));
  spdlog::info("bc: {:016b} = {}", regs.get(Reg16::BC), regs.get(Reg16::BC));
  spdlog::info("de: {:016b} = {}", regs.get(Reg16::DE), regs.get(Reg16::DE));
  spdlog::info("hl: {:016b} = {}", regs.get(Reg16::HL), regs.get(Reg16::HL));

  cpu.execute();

  spdlog::info("Exiting.");

  return 0;
}
