#pragma once

#include <raylib.h>

#include "immu.h"

enum class PPUMode {
  HBlank = 0,
  VBlank = 1,
  OAM = 2,
  Draw = 3,
};

class PPU {
public:
  void init();
  void cleanup();
  void execute(IMMU *mmu, uint8_t cycles);
  void step(IMMU *mmu);
  void reset();

  RenderTexture2D target;

private:
  uint16_t cycle_counter = 0;
};
