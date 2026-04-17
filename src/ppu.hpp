#pragma once

#include <array>
#include <raylib.h>

#include "types.hpp"
#include "mmu.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"
#include "cpu_state.hpp"


constexpr size_t kNumTiles = 384;
constexpr size_t kVramNumBanks = 2;
constexpr size_t kCgbNumPalettes = 8;

using Palette = std::array<Color, 4>;

enum class PPUMode : u8 {
  HBlank = 0,
  VBlank = 1,
  OAM = 2,
  Draw = 3,
};

struct PpuRegs {
  struct LCDC {
    u8 bg_window_enable;
    u8 sprite_enable;
    u8 sprite_size;
    u8 bg_tilemap_area;
    u8 tiledata_area;
    u8 window_enable;
    u8 window_tilemap_area;
    u8 lcd_enable;

    LCDC(u8 val) {
      bg_window_enable = val & 0x1;
      sprite_enable = (val >> 1) & 0x1;
      sprite_size = (val >> 2) & 0x1;
      bg_tilemap_area = (val >> 3) & 0x1;
      tiledata_area = (val >> 4) & 0x1;
      window_enable = (val >> 5) & 0x1;
      window_tilemap_area = (val >> 6) & 0x1;
      lcd_enable = (val >> 7) & 0x1;
    }

    u8 byte() const {
      return (lcd_enable << 7) | (window_tilemap_area << 6) | (window_enable << 5) | (tiledata_area << 4) | (bg_tilemap_area << 3) | (sprite_size << 2) | (sprite_enable << 1) | bg_window_enable;
    }
  };

  struct Stat {
    u8 ppu_mode: 2;
    u8 coincidence_flag: 1;
    u8 stat_interrupt_mode0: 1;
    u8 stat_interrupt_mode1: 1;
    u8 stat_interrupt_mode2: 1;
    u8 stat_interrupt_lyc: 1;
    u8 : 1;

    Stat(u8 val) {
      ppu_mode = val & 0x3;
      coincidence_flag = (val >> 2) & 0x1;
      stat_interrupt_mode0 = (val >> 3) & 0x1;
      stat_interrupt_mode1 = (val >> 4) & 0x1;
      stat_interrupt_mode2 = (val >> 5) & 0x1;
      stat_interrupt_lyc = (val >> 6) & 0x1;
    }

    u8 byte() const {
      return 0x80 | (stat_interrupt_lyc << 6) | (stat_interrupt_mode2 << 5) | (stat_interrupt_mode1 << 4) | (stat_interrupt_mode0 << 3) | (coincidence_flag << 2) | ppu_mode;
    }
  };

  PpuRegs::LCDC lcdc = 0;
  Stat stat = 0;
  u8 scy;
  u8 scx;
  u8 ly;
  u8 lyc;
  u8 dma;
  u8 bgp;
  u8 obp0;
  u8 obp1;
  u8 wy;
  u8 wx;

  void Reset() {
    lcdc = 0;
    stat = 0;
    scy = 0;
    scx = 0;
    ly = 0;
    lyc = 0;
    dma = 0;
    bgp = 0;
    obp0 = 0;
    obp1 = 0;
    wy = 0;
    wx = 0;
  }
};

struct CgbColor {
  union {
    u16 value;
    struct {
      u8 lo : 8;
      u8 hi : 8;
    };
  };

  CgbColor() = default;
  CgbColor(u16 val):value{val} {}

  Color GetColor() const {
    u8 r = (value & 0x1f) << 3;
    u8 g = ((value >> 5) & 0x1f) << 3;
    u8 b = ((value >> 10) & 0x1f) << 3;
    return Color{ .r = r, .g = g, .b = b, .a = 0xff };
  }
};

struct CgbPpuRegs {
  union {
    u8 val;
    struct {
      u8 address : 6;
      u8 : 1;
      u8 auto_increment : 1;
    };
  } bcps {};

  union {
    u8 val;
    struct {
      u8 address : 6;
      u8 : 1;
      u8 auto_increment : 1;
    };
  } ocps {};

  std::array<CgbColor, 32> bcpd {};
  std::array<CgbColor, 32> ocpd {};

  CgbPpuRegs() = default;

  void Reset() {
    bcps.val = 0;
    ocps.val = 0;
    bcpd.fill(0);
    ocpd.fill(0);
  }
};

struct Sprite {
  u8 y;
  u8 x;
  u8 tile;
  u8 attrs;
};

struct SpriteAttrs {
  u8 cgb_palette : 3;
  u8 cgb_bank    : 1;
  u8 dmg_palette : 1;
  u8 x_flip      : 1;
  u8 y_flip      : 1;
  u8 priority    : 1;

  SpriteAttrs(u8 val) {
    cgb_palette = static_cast<u8>(val & 0x7);
    cgb_bank = static_cast<u8>((val >> 3) & 0x1);
    dmg_palette = static_cast<u8>((val >> 4) & 0x1);
    x_flip = static_cast<u8>((val >> 5) & 0x1);
    y_flip = static_cast<u8>((val >> 6) & 0x1);
    priority = static_cast<u8>((val >> 7) & 0x1);
  }
};

struct OamMemory {
  union {
    std::array<u8, 160> bytes;
    std::array<Sprite, 40> sprites;
  };

  inline void Reset() {
    bytes.fill(0);
  }
};

struct VramMemory {
  union {
    std::array<u8, 8192> bytes;
    struct {
      std::array<std::array<u16, 8>, kNumTiles> tile_data;
      std::array<std::array<u8, 1024>, 2> tile_map;
    };
  };

  inline void Reset() {
    bytes.fill(0);
  }
};

struct VramTileAttrib {
  u8 palette;
  u8 bank;
  u8 x_flip;
  u8 y_flip;
  u8 priority;

  VramTileAttrib(u8 val) {
    palette = static_cast<u8>(val & 0x7);
    bank = static_cast<u8>((val >> 3) & 0x1);
    x_flip = static_cast<u8>((val >> 5) & 0x1);
    y_flip = static_cast<u8>((val >> 6) & 0x1);
    priority = static_cast<u8>((val >> 7) & 0x1);
  }
};

struct DmaRegs {
  u16 source;
  u16 destination;
  u8 dma;
};

struct DmaState {
  bool hdma;
  u16 length;
  u16 source;
  u16 destination;
};

struct PpuConfig {
  Mmu* mmu;
  CpuState* state;
  InterruptDevice* interrupts;
  std::array<Color, 4> palette;
};

class Ppu : public MmuDevice, public SyncedDevice {
public:
  void Init(PpuConfig config);
  void Cleanup();
  void Step();

  void OnTick(bool double_speed) override;

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  [[nodiscard]] PPUMode GetMode() const;
  [[nodiscard]] const Texture2D& GetTextureLcd() const;
  [[nodiscard]] const RenderTexture2D& GetTextureTilemap1() const;
  [[nodiscard]] const RenderTexture2D& GetTextureTilemap2() const;
  [[nodiscard]] const RenderTexture2D& GetTextureSprites() const;
  [[nodiscard]] const RenderTexture2D& GetTextureTiles() const;
  [[nodiscard]] const RenderTexture2D& GetTexturePalettes() const;

  void ClearTargetBuffers();
  void UpdateRenderTargets();

  void ResetFrameCount();
  size_t GetFrameCount() const;

  void UpdatePalette(std::array<Color, 4> palette);

private:
  void SetMode(PPUMode mode);
  void DrawLcdRow();
  void SwapLcdTargets();
  void StartDma();
  void StartGPDma();
  void StartHBlankDma();

  VramMemory& Bank();
  const VramMemory& Bank() const;
  const VramMemory& BankAt(u8 bit) const;

private:
  Mmu* mmu_ = nullptr;
  CpuState* state_ = nullptr;
  InterruptDevice* interrupts_ = nullptr;
  Texture2D target_lcd_front_ {};
  Image target_lcd_back_ {};

  RenderTexture2D target_tilemap1_ {};
  RenderTexture2D target_tilemap2_ {};
  RenderTexture2D target_sprites_ {};
  RenderTexture2D target_tiles_ {};
  RenderTexture2D target_palettes_ {};

  std::array<VramMemory, kVramNumBanks> banks_ {};
  std::array<Palette, kCgbNumPalettes> cgb_bg_palettes_ {};
  std::array<Palette, kCgbNumPalettes> cgb_sprite_palettes_ {};
  OamMemory oam_ {};
  PpuRegs regs_ {};
  u8 vbk_ {};
  Palette palette_ {};
  DmaRegs dma_regs_ {};
  DmaState dma_state_ {};
  CgbPpuRegs cgb_regs_ {};
  u8 opri_ {};

  size_t frame_count_ = 0;
  u16 cycle_counter_ = 0;
  u8 window_line_counter_ = 0;
  bool log_doctor_ = false;
  u8 tick_counter_ = 0;
};
