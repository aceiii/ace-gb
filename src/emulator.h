#pragma once

#include <vector>

#include "cpu.h"
#include "registers.h"

class Emulator {
public:
  Emulator();

  bool initialize();
  void update();
  void cleanup();

  void load_cartridge(const std::vector<uint8_t> &bytes);

  void reset();
  void step();
  void play();
  void stop();

  bool is_playing() const;

  const Registers& registers() const;
  size_t cycles() const;

private:
  CPU cpu;

  size_t num_cycles = 0;
  bool running = false;
};
