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

inline uint16_t addr_with_mode(uint8_t mode, uint8_t addr) {
//  spdlog::info("addr_with_mode({}): {} -> {}", mode, addr, mode ? addr_mode_8000(addr) : addr_mode_8800(addr));
  return mode ? addr_mode_8000(addr * 16) : addr_mode_8800(addr * 16);
}

Ppu::Ppu(InterruptDevice &interrupts_):interrupts{interrupts_} {
}

void Ppu::init() {
  for (int i = 0; i < targets.size(); i += 1) {
    targets[i] = LoadRenderTexture(kLCDWidth, kLCDHeight);
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
  if (!regs.lcdc.lcd_enable) {
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
  if (!regs.lcdc.lcd_enable) {
    return;
  }
}

void Ppu::update_render_targets() {
  constexpr int tile_width = 16;
  constexpr int tile_height = 24;

  BeginTextureMode(target_tiles);
  {

    /*
    // NOTE: Test tile data and palette
    regs.bgp.id0 = 3;
    regs.bgp.id1 = 2;
    regs.bgp.id2 = 1;
    regs.bgp.id3 = 0;

    for (int i = 0; i < 78; i += 1) {
      int b = i * 32;
      vram.bytes[b +  0] = 0x3c; vram.bytes[b +  1] = 0x7e;
      vram.bytes[b +  2] = 0x42; vram.bytes[b +  3] = 0x42;
      vram.bytes[b +  4] = 0x42; vram.bytes[b +  5] = 0x42;
      vram.bytes[b +  6] = 0x42; vram.bytes[b +  7] = 0x42;
      vram.bytes[b +  8] = 0x7e; vram.bytes[b +  9] = 0x5e;
      vram.bytes[b + 10] = 0x7e; vram.bytes[b + 11] = 0x0a;
      vram.bytes[b + 12] = 0x7c; vram.bytes[b + 13] = 0x56;
      vram.bytes[b + 14] = 0x38; vram.bytes[b + 15] = 0x7c;

      vram.bytes[b + 16] = 0xff; vram.bytes[b + 17] = 0x00;
      vram.bytes[b + 18] = 0x7e; vram.bytes[b + 19] = 0xff;
      vram.bytes[b + 20] = 0x85; vram.bytes[b + 21] = 0x81;
      vram.bytes[b + 22] = 0x89; vram.bytes[b + 23] = 0x83;
      vram.bytes[b + 24] = 0x93; vram.bytes[b + 25] = 0x85;
      vram.bytes[b + 26] = 0xa5; vram.bytes[b + 27] = 0x8b;
      vram.bytes[b + 28] = 0xc9; vram.bytes[b + 29] = 0x97;
      vram.bytes[b + 30] = 0x7e; vram.bytes[b + 31] = 0xff;
    }
    */

    int x = 0;
    int y = 0;
    for (auto &tile : vram.tile_data) {
      for (int row = 0; row < tile.size(); row += 1) {
        uint16_t hi = (tile[row] >> 8) << 1;
        uint8_t lo = tile[row];
        for (int b = 7; b >= 0; b -= 1) {
          uint8_t bits = (hi & 0b10) | (lo & 0b1);

//          uint8_t id;
//          switch (bits) {
//            case 0: id = regs.bgp.id0; break;
//            case 1: id = regs.bgp.id1; break;
//            case 2: id = regs.bgp.id2; break;
//            case 3: id = regs.bgp.id3; break;
//            default: std::unreachable();
//          }

          auto color = kLCDPalette[bits];
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
  }
  EndTextureMode();

  BeginTextureMode(target_bg);
  {
    auto tilemap_area = regs.lcdc.bg_tilemap_area;
    auto tiledata_area = regs.lcdc.tiledata_area;
    auto &tilemap = vram.tile_map[tilemap_area];

    uint8_t i = 0;
    for (auto &tile_id : tilemap) {
      tile_id = i;
      i++;
    }

    int x = 0;
    int y = 0;
    for (const auto &tile_id : tilemap) {
      auto tile_idx = (addr_with_mode(tiledata_area, tile_id) - kVRAMAddrStart) / 16;
      auto dst_y = tile_idx / 16;
      auto dst_x = tile_idx % 16;

      Rectangle rect {
        static_cast<float>(dst_x * 8),
        static_cast<float>(target_tiles.texture.height - (dst_y * 8) - 8),
        8.f,
        -8.f,
      };

      Vector2 pos {
        static_cast<float>(x * 8),
        static_cast<float>(y * 8),
      };
      DrawTextureRec(target_tiles.texture, rect, pos, WHITE);

      x += 1;
      if (x >= 32) {
        x = 0;
        y += 1;
      }
    }

    auto x1 = regs.scx;
    auto y1 = regs.scy;
    auto x2 = (regs.scx + kLCDWidth) % 256;
    auto y2 = (regs.scy + kLCDHeight) % 256;

    DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, RED);
    DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, GREEN);
    DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, YELLOW);
    DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, BLUE);
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
    spdlog::debug("Writing to VRAM: [0x{:02x}] = 0x{:02x}", addr, byte);
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
    spdlog::info("Writing to LCDC: {:02x}", byte);
    auto enable_before = regs.lcdc.lcd_enable;
    regs.lcdc.val = byte;
    if (enable_before && !regs.lcdc.lcd_enable) {
      regs.ly = 0;
      cycle_counter = 0;
      clear_target_buffers();
    }
    return;
  }

  regs.bytes[addr - std::to_underlying(IO::LCDC)] = byte;

  if (addr == std::to_underlying(IO::LYC) && byte == regs.ly) {
    interrupts.request_interrupt(Interrupt::Stat);
  }
}

[[nodiscard]] uint8_t Ppu::read8(uint16_t addr) const {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return vram.bytes[addr - kVRAMAddrStart];
  }

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
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_bg);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_bg);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_sprites);
  ClearBackground(BLACK);
  EndTextureMode();

  clear_target_buffers();
}

void Ppu::clear_target_buffers() {
  for (int i = 0; i < targets.size(); i += 1) {
    BeginTextureMode(targets[i]);
    ClearBackground(BLACK);
    EndTextureMode();
  }
}

PPUMode Ppu::mode() const {
  return static_cast<PPUMode>(regs.stat.ppu_mode);
}

