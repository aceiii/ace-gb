#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>

#include "emulator.h"
#include "mmu.h"

Emulator::Emulator() {
  cpu.mmu = std::make_unique<MMU>();
  cpu.init();
}

bool Emulator::initialize() {

  std::ifstream input("./boot.bin", std::ios::binary);
  if (input.fail()) {
    spdlog::error("Failed to load boot rom: {}", strerror(errno));
    return false;
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
    cycles += cpu.execute();
  } while (cycles < cycles_per_frame);

  num_cycles += cycles;
}

void Emulator::cleanup() {
}

void Emulator::load_cartridge(const std::vector<uint8_t> &bytes) {
  cpu.mmu->load_cartridge(bytes);
}

void Emulator::reset() {
  cpu.reset();
  cpu.mmu->reset();
}

void Emulator::step() {
  if (running) {
    return;
  }

  // TODO: single single
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
