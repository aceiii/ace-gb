#pragma once

#include <array>
#include <raylib.h>

#include "types.hpp"
#include "mmu.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"
#include "cpu_state.hpp"
#include "hardware_mode.hpp"


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
  union {
    std::array<u8, 12> bytes;
    struct {
      union {
        struct {
          u8 bg_window_enable: 1;
          u8 sprite_enable: 1;
          u8 sprite_size: 1;
          u8 bg_tilemap_area: 1;
          u8 tiledata_area: 1;
          u8 window_enable: 1;
          u8 window_tilemap_area: 1;
          u8 lcd_enable: 1;
        };
        u8 val;
      } lcdc;

      union {
        struct {
          u8 ppu_mode: 2;
          u8 coincidence_flag: 1;
          u8 stat_interrupt_mode0: 1;
          u8 stat_interrupt_mode1: 1;
          u8 stat_interrupt_mode2: 1;
          u8 stat_interrupt_lyc: 1;
          u8 : 1;
        };
        u8 val;
      } stat;

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
    };
  };

  inline void reset() {
    lcdc.val = 0;
    stat.val = 0;
    scy = 0;
    scx = 0;
    ly = 0;
    lyc = 0;
  }
};

union CgbColor {
  u16 value;
  struct {
    u8 lo : 8;
    u8 hi : 8;
  };
  struct {
    u8 red : 5;
    u8 green : 5;
    u8 blue : 5;
    u8 : 1;
  };
};

struct CgbPpuRegs {
  union {
    u8 val;
    struct {
      u8 address : 6;
      u8 : 1;
      u8 auto_increment : 1;
    };
  } bcps;

union {
    u8 val;
    struct {
      u8 address : 6;
      u8 : 1;
      u8 auto_increment : 1;
    };
  } ocps;

  std::array<CgbColor, 32> bcpd;
  std::array<CgbColor, 32> ocpd;
};

struct Sprite {
  u8 y;
  u8 x;
  u8 tile;
  union {
    struct {
      u8 cgb_palette : 3;
      u8 cgb_bank    : 1;
      u8 dmg_palette : 1;
      u8 x_flip      : 1;
      u8 y_flip      : 1;
      u8 priority    : 1;
    };
    u8 val;
  } attrs;
};

struct OamMemory {
  union {
    std::array<u8, 160> bytes;
    std::array<Sprite, 40> sprites;
  };

  inline void reset() {
    bytes.fill(0);
  }
};

union VramTileAttrib {
  u8 tile_id;
  struct {
    u8 palette : 3;
    u8 bank : 1;
    u8 : 1;
    u8 x_flip : 1;
    u8 y_flip : 1;
    u8 priority : 1;
  };
};

struct VramMemory {
  union {
    std::array<u8, 8192> bytes;
    struct {
      std::array<std::array<u16, 8>, kNumTiles> tile_data;
      std::array<std::array<VramTileAttrib, 1024>, 2> tile_map;
    };
  };

  inline void reset() {
    bytes.fill(0);
  }
};

struct VramBankReg {
  union {
    u8 val;
    struct {
      u8 bank: 1;
      u8 : 7;
    };
  };
};

struct DmaRegs {
  union {
    struct {
      u8 low;
      u8 high;
    };
    u16 val;
  } source;

  union {
    struct {
      u8 low;
      u8 high;
    };
    u16 val;
  } destination;

  union {
    u8 val;
    struct {
      u8 length : 7;
      u8 mode_active : 1;
    };
  } dma;
};

struct DmaState {
  u16 length;
  u16 destination;
  u16 source;
  bool hdma;
  bool active;
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
  void Step(int n = 0);

  void OnTick() override;

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

  void SetHardwareMode(HardwareMode mode);
  HardwareMode GetHardwareMode() const;

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

  HardwareMode hardware_mode_ = HardwareMode::kDmgMode;
  std::array<VramMemory, kVramNumBanks> banks_ {};
  std::array<Palette, kCgbNumPalettes> cgb_bg_palettes_ {};
  std::array<Palette, kCgbNumPalettes> cgb_sprite_palettes_ {};
  OamMemory oam_ {};
  PpuRegs regs_ {};
  VramBankReg vbk_ {};
  Palette palette_ {};
  DmaRegs dma_regs_ {};
  DmaState dma_state_ {};
  CgbPpuRegs cgb_regs_ {};

  size_t frame_count_ = 0;
  u16 cycle_counter_ = 0;
  u8 window_line_counter_ = 0;
  bool log_doctor_ = false;
};
