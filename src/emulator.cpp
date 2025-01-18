#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "file.h"
#include "mmu.h"

Emulator::Emulator() = default;

tl::expected<bool, std::string> Emulator::init() {
  cpu.init();
  ppu.init();

  mmu.clear_devices();
  mmu.add_device(&boot_rom);
  mmu.add_device(&ppu);

//  mmu->on_write8(std::to_underlying(IO::DIV), [](uint16_t addr, uint8_t val) {
//    return 0;
//  });
//
//  mmu->on_write8(std::to_underlying(IO::TAC), [&](uint16_t addr, uint8_t tac) {
//    update_tima(tac);
//    return tac;
//  });

  auto result = load_bin("./boot.bin");
  if (!result) {
    return tl::unexpected(fmt::format("Failed to load boot rom: {}", result.error()));
  }

  auto rom_bytes = result.value();
  spdlog::info("Loading boot ROM data. {}", rom_bytes.size());
  boot_rom.load_bytes(rom_bytes);

  spdlog::info("Starting Cpu.");

  return true;
}

void Emulator::update() {
  if (!running && !cpu.state.halt) {
    return;
  }

  const auto fps = 60;
  const auto cycles_per_frame = kClockSpeed / fps;
  int cycles = 0;

  do {
    cycles += cpu.execute(mmu);
    execute_timers(cycles);
    ppu.execute(cycles);
  } while (cycles < cycles_per_frame);

  num_cycles += cycles;
}

void Emulator::cleanup() {
  ppu.cleanup();
}

void Emulator::load_cartridge(const std::vector<uint8_t> &bytes) {
//  mmu->load_cartridge(bytes);
}

void Emulator::reset() {
  cpu.reset();
  mmu.reset_devices();
  boot_rom.reset();

  div_counter = 0;
  tima_counter = 0;
}

void Emulator::step() {
  if (running) {
    return;
  }

  auto cycles = cpu.execute(mmu);
  execute_timers(cycles);
  ppu.execute(cycles);

  num_cycles += cycles;
}

void Emulator::play() {
  running = true;
}

void Emulator::stop() {
  running = false;
}

bool Emulator::is_playing() const {
  return running;
}

const Registers& Emulator::registers() const {
  return cpu.regs;
}

size_t Emulator::cycles() const {
  return num_cycles;
}

void Emulator::render() {
  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  ImGui::Begin("GameBoy");
  {
    rlImGuiImageRenderTextureFit(ppu.target(), true);
  }
  ImGui::End();
}

void Emulator::execute_timers(uint8_t cycles) {
//  div_counter += cycles;
//  if (div_counter >= 256) {
//    div_counter -= 256;
//    mmu->inc(std::to_underlying(IO::DIV));
//  }
//
//  auto tac = mmu->read8(std::to_underlying(IO::TAC));
//  if ((tac >> 2) & 0x1) {
//    tima_counter += cycles;
//    update_tima(tac);
//  }
//
//  current_tac = tac;
}

void Emulator::update_tima(uint8_t tac) {
//  static uint16_t timer_freqs[] = {
//    kClockSpeed / 4096,
//    kClockSpeed / 262144,
//    kClockSpeed / 65535,
//    kClockSpeed / 16384,
//  };
//
//  auto freq = timer_freqs[tac & 0x3];
//  if (tima_counter >= freq) {
//    uint8_t tima = mmu->read8(std::to_underlying(IO::TIMA));
//    if (tima == 255) {
//      mmu->write(std::to_underlying(IO::TIMA), mmu->read8(std::to_underlying(IO::TMA)));
//      uint8_t disabled_bits = mmu->read8(std::to_underlying(IO::IE)) & ~(1 << std::to_underlying(Interrupt::Timer));
//      mmu->write(std::to_underlying(IO::IE), disabled_bits);
//    } else {
//      tima += 1;
//      mmu->write(std::to_underlying(IO::TIMA), tima);
//    }
//    tima_counter -= freq;
//  }
};

PPUMode Emulator::mode() const {
  return ppu.mode();
}

Instruction Emulator::instr() const {
  auto byte = mmu.read8(cpu.regs.pc);
  auto instr = Decoder::decode(byte);
  if (instr.opcode == Opcode::PREFIX) {
    byte = mmu.read8(cpu.regs.pc + 1);
    instr = Decoder::decode_prefixed(byte);
  }
  return instr;
}

uint8_t Emulator::read8(uint16_t addr) const {
  return mmu.read8(addr);
}
