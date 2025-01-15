#pragma once

#include <memory>
#include <vector>

#include "cpu.h"
#include "ppu.h"
#include "registers.h"

class Emulator {
public:
  Emulator();

  bool init();
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
  CPU cpu;
  MMU mmu;
  PPU ppu;

  size_t num_cycles = 0;
  bool running = false;

  uint16_t div_counter = 0;
  uint16_t tima_counter = 0;
  uint8_t current_tac = 0;

  void execute_timers(uint8_t cycles);
  void update_tima(uint8_t tac);
};
