#include <cstdint>
#include <utility>
#include <spdlog/spdlog.h>

#include "ppu.h"

namespace {

constexpr uint16_t kLCDWidth = 160;
constexpr uint16_t kLCDHeight = 144;

constexpr uint16_t kVRAMAddrStart = 0x8000;
constexpr uint16_t kVRAMAddrEnd = 0x9FFF;
constexpr uint16_t kVRAMRelStart = 0x9000;

constexpr uint16_t kOAMAddrStart = 0xFE00;
constexpr uint16_t kOAMAddrEnd = 0xFE9F;

constexpr size_t kDotsPerOAM = 80;
constexpr size_t kDotsPerDraw = 172;
constexpr size_t kDotsPerRow = 456;

constexpr std::array<Color, 4> kLCDPalette {
  Color { 223, 247, 207, 255 },
  Color { 135, 192, 111, 255 },
  Color { 51, 104, 85, 255 },
  Color { 8, 23, 32, 255 },
};

}

inline uint8_t get_palette_index(uint8_t id, ppu_regs &regs) {
  switch (id) {
    case 0: return regs.bgp.id0;
    case 1: return regs.bgp.id1;
    case 2: return regs.bgp.id2;
    case 3: return regs.bgp.id3;
    default: std::unreachable();
  }
}

inline uint16_t addr_mode_8000(uint8_t addr) {
  return kVRAMAddrStart + addr * 16;
}

inline uint16_t addr_mode_8800(uint8_t addr) {
  return kVRAMRelStart + (static_cast<int8_t>(addr) * 16);
}

inline uint16_t addr_with_mode(uint8_t mode, uint8_t addr) {
  return mode ? addr_mode_8000(addr) : addr_mode_8800(addr);
}

Ppu::Ppu(InterruptDevice &interrupts_)
:interrupts{interrupts_}
{}

void Ppu::init() {
  target_lcd_back = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  target_lcd_front = LoadTextureFromImage(target_lcd_back);

  constexpr int tiles_width = 16 * 8;
  constexpr int tiles_height = 24 * 8;
  target_tiles = LoadRenderTexture(tiles_width, tiles_height);

  constexpr int tilemap_width = 256;
  constexpr int tilemap_height = 256;
  target_tilemap1 = LoadRenderTexture(tilemap_width, tilemap_height);
  target_tilemap2 = LoadRenderTexture(tilemap_width, tilemap_height);
  target_sprites = LoadRenderTexture(tilemap_width, tilemap_height);

  target_lcd_back = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

void Ppu::cleanup() {
  UnloadRenderTexture(target_tiles);
  UnloadRenderTexture(target_tilemap1);
  UnloadRenderTexture(target_tilemap2);
  UnloadRenderTexture(target_sprites);

  UnloadImage(target_lcd_back);
  UnloadTexture(target_lcd_front);
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

  cycle_counter += 1;

  if (cycle_counter >= kDotsPerRow) {
    regs.ly += 1;
    cycle_counter = 0;
  }

  if (regs.ly >= kLCDHeight + 10) {
    regs.ly = 0;
  }

  if (regs.ly >= kLCDHeight) {
    if (mode != PPUMode::VBlank) {
      regs.stat.ppu_mode = std::to_underlying(PPUMode::VBlank);
      interrupts.request_interrupt(Interrupt::VBlank);
      if (regs.stat.stat_interrupt_mode1) {
        interrupts.request_interrupt(Interrupt::Stat);
      }
      swap_lcd_targets();
    }
  } else {
    if (cycle_counter < kDotsPerOAM) {
      if (mode != PPUMode::OAM) {
        regs.stat.ppu_mode = std::to_underlying(PPUMode::OAM);
        if (regs.stat.stat_interrupt_mode2) {
          interrupts.request_interrupt(Interrupt::Stat);
        }
      }
    } else if (cycle_counter <= (kDotsPerOAM + kDotsPerDraw)) {
      if (mode != PPUMode::Draw) {
        regs.stat.ppu_mode = std::to_underlying(PPUMode::Draw);
      }
    } else {
      if (mode != PPUMode::HBlank) {
        regs.stat.ppu_mode = std::to_underlying(PPUMode::HBlank);
        if (regs.stat.stat_interrupt_mode0) {
          interrupts.request_interrupt(Interrupt::Stat);
        }
        draw_lcd_row();
      }
    }
  }

  if (regs.ly == regs.lyc && regs.stat.stat_interrupt_lyc) {
    interrupts.request_interrupt(Interrupt::Stat);
  }
}

void Ppu::swap_lcd_targets() {
//  std::swap(target_lcd_front, target_lcd_back);
//  BeginTextureMode(*target_lcd_back);
//  ClearBackground(BLACK);
//  EndTextureMode();

  UpdateTexture(target_lcd_front, target_lcd_back.data);
}

void Ppu::draw_lcd_row() {
//  BeginTextureMode(*target_lcd_back);
  {
    if (regs.lcdc.bg_window_enable) {
      auto enable_window = regs.lcdc.window_enable;
      auto &tilemap = vram.tile_map[enable_window ? regs.lcdc.window_tilemap_area : regs.lcdc.bg_tilemap_area];

      const uint8_t y = regs.ly;
      const uint8_t py = regs.scy + y;
      const uint8_t ty = (py >> 3 ) & 31;
      const uint8_t row = py % 8;

      for (auto x = 0; x < kLCDWidth; x += 1) {
        const uint8_t px = regs.scx + x;
        const uint8_t tx = (px >> 3) & 31;
        const uint8_t col = px % 8;

        auto map_idx = (ty * 32) + tx;
        auto tile_id = tilemap[map_idx];
        auto tile_idx = (addr_with_mode(regs.lcdc.tiledata_area, tile_id) - kVRAMAddrStart) / 16;
        auto tile = vram.tile_data[tile_idx];

        uint16_t hi = (tile[row] >> 8) >> (7 - col);
        uint8_t lo = tile[row] >> (7 - col);
        uint8_t bits = ((hi & 0b1) << 1) | (lo & 0b1);
        auto cid = get_palette_index(bits, regs);
        auto color = kLCDPalette[cid];
//        DrawPixel(x, y, color);
        ImageDrawPixel(&target_lcd_back, x, y, color);
      }
    } else {
//      DrawLine(0, regs.ly, kLCDWidth - 1, regs.ly, kLCDPalette[0]);
      ImageDrawLine(&target_lcd_back, 0, regs.ly, kLCDWidth -1, regs.ly, kLCDPalette[0]);
    }

    if (regs.lcdc.sprite_enable) {
    }
  }
//  EndTextureMode();
}

const Texture2D& Ppu::lcd() const {
  return target_lcd_front;
}

const RenderTexture2D& Ppu::tilemap1() const {
  return target_tilemap1;
}

const RenderTexture2D& Ppu::tilemap2() const {
  return target_tilemap2;
}

const RenderTexture2D& Ppu::sprites() const {
  return target_sprites;
}

const RenderTexture2D& Ppu::tiles() const {
  return target_tiles;
}

void Ppu::update_render_targets() {
  constexpr int tile_width = 16;
  constexpr int tile_height = 24;

  BeginTextureMode(target_tiles);
  {
    int x = 0;
    int y = 0;
    for (auto &tile : vram.tile_data) {
      for (int row = 0; row < tile.size(); row += 1) {
        uint16_t hi = (tile[row] >> 8) << 1;
        uint8_t lo = tile[row];
        for (int b = 7; b >= 0; b -= 1) {
          uint8_t bits = (hi & 0b10) | (lo & 0b1);
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

  BeginTextureMode(target_tilemap1);
  {
    auto tiledata_area = regs.lcdc.tiledata_area;
    auto &tilemap = vram.tile_map[0];

    int x = 0;
    int y = 0;
    for (auto tile_id : tilemap) {
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

    if (regs.lcdc.bg_tilemap_area == 0) {
      auto x1 = regs.scx;
      auto y1 = regs.scy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, RED);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, RED);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, RED);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, RED);
    }
    if (regs.lcdc.window_tilemap_area == 0) {
      auto x1 = regs.wx < 7 ? kLCDWidth + regs.wx - 7 : regs.wx - 7;
      auto y1 = regs.wy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, BLUE);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, BLUE);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, BLUE);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, BLUE);
    }
  }
  EndTextureMode();

  BeginTextureMode(target_tilemap2);
  {
    auto tilemap_area = regs.lcdc.bg_tilemap_area;
    auto tiledata_area = regs.lcdc.tiledata_area;
    auto &tilemap = vram.tile_map[1];

    int x = 0;
    int y = 0;
    for (auto tile_id : tilemap) {
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

    if (regs.lcdc.bg_tilemap_area == 1) {
      auto x1 = regs.scx;
      auto y1 = regs.scy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, RED);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, RED);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, RED);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, RED);
    }
    if (regs.lcdc.window_tilemap_area == 1) {
      auto x1 = regs.wx < 7 ? kLCDWidth + (regs.wx - 7) : regs.wx - 7;
      auto y1 = regs.wy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, BLUE);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, BLUE);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, BLUE);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, BLUE);
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
  cycle_counter = 0;

  BeginTextureMode(target_tiles);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_tilemap1);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_tilemap2);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_sprites);
  ClearBackground(BLACK);
  EndTextureMode();

  clear_target_buffers();
}

void Ppu::clear_target_buffers() {
  ImageClearBackground(&target_lcd_back, BLACK);
  UpdateTexture(target_lcd_front, target_lcd_back.data);
}

PPUMode Ppu::mode() const {
  return static_cast<PPUMode>(regs.stat.ppu_mode);
}

void Ppu::set_mode(PPUMode mode) {
  regs.stat.ppu_mode = std::to_underlying(mode);
}
