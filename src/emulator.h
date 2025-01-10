#pragma once

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

  void load_cartridge(const std::vector<uint8_t> &bytes);

  void reset();
  void step();
  void play();
  void stop();

  bool is_playing() const;

  const Registers& registers() const;
  size_t cycles() const;

  void render();

private:
  CPU cpu;
  PPU ppu;

  size_t num_cycles = 0;
  bool running = false;
};
