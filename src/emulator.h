#pragma once

#include <memory>
#include <string>
#include <vector>
#include <tl/expected.hpp>

#include "cpu.h"
#include "ppu.h"
#include "registers.h"
#include "boot_rom_device.h"
#include "cart_device.h"
#include "wram_device.h"
#include "timer.h"

class Emulator {
public:
  Emulator();

  tl::expected<bool, std::string> init();
  void cleanup();
  void update();
  void reset();
  void step();
  void play();
  void stop();
  void render();

  void load_cartridge(const std::vector<uint8_t> &bytes);

  [[nodiscard]] bool is_playing() const;

  [[nodiscard]] const Registers& registers() const;
  [[nodiscard]] size_t cycles() const;
  [[nodiscard]] PPUMode mode() const;
  [[nodiscard]] Instruction instr() const;
  [[nodiscard]] uint8_t read8(uint16_t addr) const;

private:
  Mmu mmu;
  Cpu cpu;
  Ppu ppu;
  BootRomDevice boot;
  CartDevice cart;
  WramDevice wram;
  Timer timer;

  std::array<uint8_t, kBootRomSize> boot_rom;
  std::vector<uint8_t> cart_bytes;

  size_t num_cycles = 0;
  bool running = false;
};
