#include <cstdint>

#include "ppu.h"

const int kLCDWidth = 160;
const int kLCDHeight = 144;

void PPU::init() {
  target = LoadRenderTexture(kLCDWidth, kLCDHeight);

  BeginTextureMode(target);
  ClearBackground(Color(75, 0, 1, 255));
  EndTextureMode();
}

void PPU::cleanup() {
  UnloadRenderTexture(target);
}

void PPU::reset() {
}

void PPU::execute(IMMU *mmu, uint8_t cycles) {
  while (cycles--) {
    step(mmu);
  }
}

inline void PPU::step(IMMU *mmu) {
}
