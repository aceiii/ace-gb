#pragma once

#include <array>
#include <raylib.h>

#include "mmu.h"

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

struct ppu_regs {
  union {
    struct {
      uint8_t bg_window_enable      : 1;
      uint8_t sprite_enable         : 1;
      uint8_t sprite_size           : 1;
      uint8_t bg_tilemap_select     : 1;
      uint8_t tile_data_select      : 1;
      uint8_t window_display_enable : 1;
      uint8_t windw_tilemap_select  : 1;
      uint8_t lcd_display_enable    : 1;
    };
    uint8_t val;
  } lcdc;

  union {
    struct {
      uint8_t ppu_mode              : 2;
      uint8_t coincidence_flag      : 1;
      uint8_t stat_interrupt_mode0  : 1;
      uint8_t stat_interrupt_mode1  : 1;
      uint8_t stat_interrupt_mode2  : 1;
      uint8_t stat_interrupt_lyc_ly : 1;
      uint8_t unused                : 1;
    };
    uint8_t val;
  } stat;

  uint8_t scy;
  uint8_t scx;
  uint8_t ly;
  uint8_t lyc;
};

class PPU : public IMMUDevice {
public:
  void init();
  void cleanup();
  void execute(uint8_t cycles);
  void step();

  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  [[nodiscard]] PPUMode mode() const;
  [[nodiscard]] const RenderTexture2D* target() const;

private:
  void populate_sprite_buffer();

private:
  std::array<RenderTexture2D, 2> targets;
  std::array<uint16_t, 10> sprites;

  ppu_regs regs;
  uint8_t num_sprites = 0;
  uint8_t target_index = 0;
  uint16_t cycle_counter = 0;

};
