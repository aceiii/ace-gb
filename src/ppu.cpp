#include <cstdint>
#include <utility>
#include <spdlog/spdlog.h>

#include "ppu.h"
#include "cpu.h"

constexpr uint16_t kLCDWidth = 160;
constexpr uint16_t kLCDHeight = 144;

constexpr uint16_t kVRAMAddrStart = 0x8000;
constexpr uint16_t kVRAMAddrEnd = 0x9FFF;

constexpr uint16_t kOAMAddrStart = 0xFE00;
constexpr uint16_t kOAMAddrEnd = 0xFE9F;

//constexpr std::array<Color, 4> kLCDPalette {
//  Color { 240, 240, 240, 255 },
//  Color { 178, 178, 178, 255 },
//  Color { 100, 100, 100, 255 },
//  Color { 48, 48, 48, 255 },
//};

constexpr std::array<Color, 4> kLCDPalette {
  Color { 223, 247, 207, 255 },
  Color { 135, 192, 111, 255 },
  Color { 51, 104, 85, 255 },
  Color { 8, 23, 32, 255 },
};

inline uint16_t addr_mode_8000(uint8_t addr) {
  return 0x8000 + addr;
}

inline uint16_t addr_mode_8800(uint8_t addr) {
  return 0x9000 + static_cast<int8_t>(addr);
}

Ppu::Ppu(InterruptDevice &interrupts_):interrupts{interrupts_} {
}

void Ppu::init() {
  for (int i = 0; i < targets.size(); i += 1) {
    targets[i] = LoadRenderTexture(kLCDWidth, kLCDHeight);
    BeginTextureMode(targets[i]);
    ClearBackground(Color(75, 0, 1, 255));
    EndTextureMode();
  }

  constexpr int tiles_width = 16 * 8;
  constexpr int tiles_height = 24 * 8;
  target_tiles = LoadRenderTexture(tiles_width, tiles_height);

  constexpr int tilemap_width = 256;
  constexpr int tilemap_height = 256;
  target_bg = LoadRenderTexture(tilemap_width, tilemap_height);
  target_window = LoadRenderTexture(tilemap_width, tilemap_height);
  target_sprites = LoadRenderTexture(tilemap_width, tilemap_height);
}

void Ppu::cleanup() {
  for (int i = 0; i < targets.size(); i += 1) {
    UnloadRenderTexture(targets[i]);
  }
}

void Ppu::execute(uint8_t cycles) {
  do {
    step();
  } while(--cycles);
}

inline void Ppu::step() {
  if (!regs.lcdc.lcd_display_enable) {
    return;
  }

  auto mode = this->mode();

  if (mode == PPUMode::OAM && cycle_counter == 0) {
    populate_sprite_buffer();
  } else if (mode == PPUMode::Draw && cycle_counter == 80) {
  }

  cycle_counter += 1;

  switch (mode) {
    case PPUMode::OAM:
      if (cycle_counter >= 80) {
        mode = PPUMode::Draw;
      }
      break;
    case PPUMode::Draw: break;
    case PPUMode::HBlank:
      if (cycle_counter >= 456) {
        cycle_counter = 0;
        regs.ly += 1;
        PPUMode new_mode;
        if (regs.ly >= kLCDHeight) {
          new_mode = PPUMode::VBlank;
          interrupts.request_interrupt(Interrupt::VBlank);
          target_index = target_index % targets.size();
        } else {
          new_mode = PPUMode::OAM;
        }
        mode = new_mode;
      }
      break;
    case PPUMode::VBlank:
      if (cycle_counter >= 456) {
        cycle_counter = 0;
        regs.ly += 1;

        if (regs.ly >= kLCDHeight + 10) {
          regs.ly = 0;
          mode = PPUMode::OAM;
        }
      }
      break;
  }

  if (regs.ly == regs.lyc) {
    interrupts.request_interrupt(Interrupt::Stat);
  }
}

const RenderTexture2D& Ppu::lcd() const {
  return targets[target_index];
}

const RenderTexture2D& Ppu::bg() const {
  return target_bg;
}

const RenderTexture2D& Ppu::window() const {
  return target_window;
}

const RenderTexture2D& Ppu::sprites() const {
  return target_sprites;
}

const RenderTexture2D& Ppu::tiles() const {
  return target_tiles;
}

void Ppu::populate_sprite_buffer() {
  if (!regs.lcdc.lcd_display_enable) {
    return;
  }
}

void Ppu::update_render_targets() {
  constexpr int tile_width = 16;
  constexpr int tile_height = 24;

  BeginTextureMode(target_tiles);

  /*
  // NOTE: Test tile data and palette
  regs.bgp.id0 = 3;
  regs.bgp.id1 = 2;
  regs.bgp.id2 = 1;
  regs.bgp.id3 = 0;

  vram.bytes[0] = 0x3c; vram.bytes[1] = 0x7e;
  vram.bytes[2] = 0x42; vram.bytes[3] = 0x42;
  vram.bytes[4] = 0x42; vram.bytes[5] = 0x42;
  vram.bytes[6] = 0x42; vram.bytes[7] = 0x42;
  vram.bytes[8] = 0x7e; vram.bytes[9] = 0x5e;
  vram.bytes[10] = 0x7e; vram.bytes[11] = 0x0a;
  vram.bytes[12] = 0x7c; vram.bytes[13] = 0x56;
  vram.bytes[14] = 0x38; vram.bytes[15] = 0x7c;

  vram.bytes[16] = 0xff; vram.bytes[17] = 0x00;
  vram.bytes[18] = 0x7e; vram.bytes[19] = 0xff;
  vram.bytes[20] = 0x85; vram.bytes[21] = 0x81;
  vram.bytes[22] = 0x89; vram.bytes[23] = 0x83;
  vram.bytes[24] = 0x93; vram.bytes[25] = 0x85;
  vram.bytes[26] = 0xa5; vram.bytes[27] = 0x8b;
  vram.bytes[28] = 0xc9; vram.bytes[29] = 0x97;
  vram.bytes[30] = 0x7e; vram.bytes[31] = 0xff;
  */

  int x = 0;
  int y = 0;
  for (auto &tile : vram.tile_data) {
    for (int row = 0; row < tile.size(); row += 1) {
      uint16_t hi = (tile[row] >> 8) << 1;
      uint8_t lo = tile[row];
      for (int b = 7; b >= 0; b -= 1) {
        uint8_t bits = (hi & 0b10) | (lo & 0b1);

        uint8_t id;
        switch (bits) {
          case 0: id = regs.bgp.id0; break;
          case 1: id = regs.bgp.id1; break;
          case 2: id = regs.bgp.id2; break;
          case 3: id = regs.bgp.id3; break;
          default: std::unreachable();
        }

        auto color = kLCDPalette[id];
        DrawPixel((x * 8) + b, (y * 8) + row, color);

        hi >>= 1;
        lo >>= 1;
      }
    }

    x += 1;
    if (x >= tile_width) {
      x = 0;
      y += 1;
    }
  }

  EndTextureMode();
}

bool Ppu::valid_for(uint16_t addr) const {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return true;
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    return true;
  }

  if (addr >= std::to_underlying(IO::LCDC) && addr <= std::to_underlying(IO::WX)) {
    return true;
  }

  return false;
}

void Ppu::write8(uint16_t addr, uint8_t byte) {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    vram.bytes[addr - kVRAMAddrStart] = byte;
    return;
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    oam.bytes[addr - kOAMAddrStart] = byte;
    return;
  }

  if (addr == std::to_underlying(IO::LY)) {
    return;
  }

  if (addr == std::to_underlying(IO::STAT)) {
    regs.stat.val = (regs.stat.val & 0b11) | (byte & ~0b11);
    return;
  }

  if (addr == std::to_underlying(IO::LCDC)) {
    auto enable_before = regs.lcdc.lcd_display_enable;
    regs.lcdc.val = byte;
    if (enable_before && !regs.lcdc.lcd_display_enable) {
      regs.ly = 0;
      cycle_counter = 0;
    }
    return;
  }

  regs.bytes[addr - std::to_underlying(IO::LCDC)] = byte;

  if (addr == std::to_underlying(IO::LYC) && byte == regs.ly) {
    interrupts.request_interrupt(Interrupt::Stat);
  }
}

[[nodiscard]] uint8_t Ppu::read8(uint16_t addr) const {
  if (addr >= kOAMAddrStart && addr < kOAMAddrEnd) {
    return oam.bytes[addr - kOAMAddrStart];
  }

  return regs.bytes[addr - std::to_underlying(IO::LCDC)];
}

void Ppu::reset() {
  vram.reset();
  oam.reset();
  regs.reset();
  num_sprites = 0;
  target_index = 0;
  cycle_counter = 0;

  BeginTextureMode(target_tiles);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_bg);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_bg);
  ClearBackground(BLANK);
  EndTextureMode();

  BeginTextureMode(target_sprites);
  ClearBackground(BLANK);
  EndTextureMode();
}

PPUMode Ppu::mode() const {
  return static_cast<PPUMode>(regs.stat.ppu_mode);
}
