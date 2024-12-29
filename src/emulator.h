#pragma once

#include <vector>

#include "cpu.h"
#include "registers.h"

class Emulator {
public:
  void initialize();
  void update();
  void cleanup();

  void load_rom_bytes(const std::vector<uint8_t> &bytes);

  void reset();
  void step();
  void play();
  void stop();

  bool is_playing() const;

private:
  CPU cpu;
};
