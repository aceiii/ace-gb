#pragma once

#include <array>
#include <raylib.h>

#include "mmu.h"

enum class PPUMode : uint8_t {
  HBlank = 0,
  VBlank = 1,
  OAM = 2,
  Draw = 3,
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

  inline void reset() {
    lcdc.val = 0;
    stat.val = 0;
    scy = 0;
    scx = 0;
    ly = 0;
    lyc = 0;
  }
};

struct sprite {
  uint8_t y;
  uint8_t x;
  uint8_t tile;
  uint8_t attrs;
};

struct oam_memory {
  union {
    std::array<uint8_t, 160> bytes;
    std::array<sprite, 40> sprites;
  };

  inline void reset() {
    bytes.fill(0);
  }
};

struct vram_memory {
  union {
    std::array<uint8_t, 8192> bytes;
    struct {
      std::array<uint16_t, 384> tile_data;
      std::array<std::array<uint8_t, 1024>, 2> tile_map;
    };
  };

  inline void reset() {
    bytes.fill(0);
  }
};

class Ppu : public MmuDevice {
public:
  void init();
  void cleanup();
  void execute(uint8_t cycles);
  void step();

  bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  [[nodiscard]] PPUMode mode() const;
  [[nodiscard]] const RenderTexture2D* target() const;
  [[nodiscard]] const RenderTexture2D* bg() const;
  [[nodiscard]] const RenderTexture2D* window() const;

private:
  void populate_sprite_buffer();
  void update_tiles_target();

private:
  std::array<RenderTexture2D, 2> targets;

  RenderTexture2D target_bg;
  RenderTexture2D target_window;
  RenderTexture2D target_tiles;

  vram_memory vram;
  oam_memory oam;
  ppu_regs regs;

  uint8_t num_sprites = 0;
  uint8_t target_index = 0;
  uint16_t cycle_counter = 0;

};
