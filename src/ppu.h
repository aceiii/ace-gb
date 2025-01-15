#pragma once

#include <array>
#include <raylib.h>

#include "immu.h"

enum class PPUMode {
  HBlank = 0,
  VBlank = 1,
  OAM = 2,
  Draw = 3,
};

enum class SpriteFlagMask {
  Palette = (1 << 4),
  FlipX = (1 << 5),
  FlipY = (1 << 6),
  Priority = (1 << 7),
};

enum class LCDControlMask {
  BGWindowEnable = (1 << 0),
  SpriteEnable = (1 << 1),
  SpriteSize = (1 << 2),
  BGTileMapSelect = (1 << 3),
  TileDataSelect = (1 << 4),
  WindowDisplayEnable = (1 << 5),
  WindowTileMapSelect = (1 << 6),
  LCDDisplayEnable = (1 << 7),
};

enum class LCDStatusMask {
  PPUMode = 0b11,
};

class PPU {
public:
  void init();
  void cleanup();
  void execute(IMMU *mmu, uint8_t cycles);
  void step(IMMU *mmu);
  void reset();

  [[nodiscard]] PPUMode current_mode(IMMU *mmu) const;
  void set_mode(IMMU *mmu, PPUMode mode);

  [[nodiscard]] const RenderTexture2D* target() const;

private:
  void populate_sprite_buffer(IMMU *mmu);

private:
  std::array<RenderTexture2D, 2> targets;
  std::array<uint16_t, 10> sprites;

  uint8_t num_sprites = 0;
  uint8_t target_index = 0;
  uint16_t cycle_counter = 0;
  uint16_t scanline_y = 0;
};
