#include <utility>
#include <spdlog/spdlog.h>

#include "io.hpp"
#include "ppu.hpp"

namespace {

constexpr uint16_t kLCDWidth = 160;
constexpr uint16_t kLCDHeight = 144;

constexpr uint16_t kVRAMAddrStart = 0x8000;
constexpr uint16_t kVRAMAddrEnd = 0x9FFF;
constexpr uint16_t kVRAMRelStart = 0x9000;

constexpr uint16_t kOAMAddrStart = 0xFE00;
constexpr uint16_t kOAMAddrEnd = 0xFE9F;

constexpr uint16_t kExtRamBusStart = 0xA000;
constexpr uint16_t kExtRamBusEnd = 0xDFFF;
constexpr uint16_t kExtRamBusMask = kExtRamBusEnd - kExtRamBusStart;

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

inline uint8_t get_palette_index(uint8_t id, uint8_t palette) {
  return (palette >> (2 * id)) & 0b11;
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

Ppu::Ppu(Mmu &mmu, InterruptDevice &interrupts)
:interrupts_{interrupts}, mmu_{mmu}
{
  auto logger = spdlog::get("doctor_logger");
  log_doctor_ = logger != nullptr;
}

void Ppu::Init() {
  target_lcd_back_ = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  target_lcd_front_ = LoadTextureFromImage(target_lcd_back_);

  constexpr int tiles_width = 16 * 8;
  constexpr int tiles_height = 24 * 8;
  target_tiles_ = LoadRenderTexture(tiles_width, tiles_height);

  constexpr int tilemap_width = 256;
  constexpr int tilemap_height = 256;
  target_tilemap1_ = LoadRenderTexture(tilemap_width, tilemap_height);
  target_tilemap2_ = LoadRenderTexture(tilemap_width, tilemap_height);

  constexpr int sprites_width = 8 * 8;
  constexpr int sprites_height = 5 * 16;
  target_sprites_ = LoadRenderTexture(sprites_width, sprites_height);

  target_lcd_back_ = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

void Ppu::Cleanup() {
  UnloadRenderTexture(target_tiles_);
  UnloadRenderTexture(target_tilemap1_);
  UnloadRenderTexture(target_tilemap2_);
  UnloadRenderTexture(target_sprites_);

  UnloadImage(target_lcd_back_);
  UnloadTexture(target_lcd_front_);
}

void Ppu::Execute(uint8_t cycles) {
  do {
    Step();
    cycles -= 4;
  } while(cycles);
}

void Ppu::OnTick() {
  Step();
}

inline void Ppu::Step() {
  if (!regs_.lcdc.lcd_enable) {
    return;
  }

  auto mode = this->GetMode();

  if (++cycle_counter_ >= kDotsPerRow) {
    regs_.ly = (regs_.ly + 1) % (kLCDHeight + 10);
    regs_.stat.coincidence_flag = regs_.ly == regs_.lyc;
    if (regs_.stat.coincidence_flag && regs_.stat.stat_interrupt_lyc) {
      interrupts_.RequestInterrupt(Interrupt::Stat);
    }

    cycle_counter_ = 0;
  }

  if (regs_.ly >= kLCDHeight) {
    if (mode != PPUMode::VBlank) {
      window_line_counter_ = 0;
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::VBlank);
      interrupts_.RequestInterrupt(Interrupt::VBlank);
      if (regs_.stat.stat_interrupt_mode0) {
        interrupts_.RequestInterrupt(Interrupt::Stat);
      }
      SwapLcdTargets();
    }
  } else if (cycle_counter_ < kDotsPerOAM) {
    if (mode != PPUMode::OAM) {
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::OAM);
      if (regs_.stat.stat_interrupt_mode2) {
        interrupts_.RequestInterrupt(Interrupt::Stat);
      }
    }
  } else if (cycle_counter_ <= (kDotsPerOAM + kDotsPerDraw)) {
    if (mode != PPUMode::Draw) {
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::Draw);
    }
  } else {
    if (mode != PPUMode::HBlank) {
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::HBlank);
      if (regs_.stat.stat_interrupt_mode0) {
        interrupts_.RequestInterrupt(Interrupt::Stat);
      }
      DrawLcdRow();
    }
  }
}

void Ppu::SwapLcdTargets() {
  UpdateTexture(target_lcd_front_, target_lcd_back_.data);
}

void Ppu::DrawLcdRow() {
  static std::array<uint8_t, kLCDWidth> bg_win_pixels;
  bg_win_pixels.fill(0);

  if (regs_.lcdc.bg_window_enable) {
    const bool enable_window_flag = regs_.lcdc.window_enable &&
      regs_.wx >= 0 && regs_.wx <= 166 && regs_.wy >= 0 && regs_.wy <= 143;
    const uint8_t y = regs_.ly;

    uint8_t py = regs_.scy + y;
    uint8_t ty = (py >> 3 ) & 31;
    uint8_t row = py % 8;

    bool enable_window = false;
    for (auto x = 0; x < kLCDWidth; x += 1) {
      if (!enable_window && enable_window_flag && x >= regs_.wx - 7 && y >= regs_.wy) {
        enable_window = true;
        py = window_line_counter_;// y - regs.wy;
        ty = (py >> 3 ) & 31;
        row = py % 8;
      }

      auto &tilemap = vram_.tile_map[enable_window ? regs_.lcdc.window_tilemap_area : regs_.lcdc.bg_tilemap_area];

      uint8_t px = enable_window ? x - (regs_.wx - 7) : regs_.scx + x;
      uint8_t tx = (px >> 3) & 31;
      uint8_t sub_x = px % 8;

      auto map_idx = (ty * 32) + tx;
      auto tile_id = tilemap[map_idx];
      auto tile_idx = (addr_with_mode(regs_.lcdc.tiledata_area, tile_id) - kVRAMAddrStart) / 16;
      auto tile = vram_.tile_data[tile_idx];

      uint16_t hi = (tile[row] >> 8) >> (7 - sub_x);
      uint8_t lo = tile[row] >> (7 - sub_x);
      uint8_t bits = ((hi & 0b1) << 1) | (lo & 0b1);

      auto cid = get_palette_index(bits, regs_.bgp);
      auto color = kLCDPalette[cid];
      ImageDrawPixel(&target_lcd_back_, x, y, color);
      bg_win_pixels[x] = bits; // NOTE: might be cid...
    }

    if (enable_window) {
      window_line_counter_++;
    }
  } else {
    auto cid = get_palette_index(0, regs_.bgp);
    auto color = kLCDPalette[cid];
    ImageDrawLine(&target_lcd_back_, 0, regs_.ly, kLCDWidth -1, regs_.ly,  color);
  }

  if (regs_.lcdc.sprite_enable) {
    static std::vector<Sprite*> valid_sprites;
    valid_sprites.clear();
    valid_sprites.reserve(10);

    const auto height = regs_.lcdc.sprite_size ? 16 : 8;
    const auto y = regs_.ly;

    for (auto &sprite : oam_.sprites) {
      auto top = sprite.y - 16;
      auto bottom = top + height;
      if (y >= top && y < bottom) {
        valid_sprites.push_back(&sprite);
        if (valid_sprites.size() >= 10) {
          break;
        }
      }
    }

    static std::array<uint8_t, kLCDWidth> sprite_prio;
    sprite_prio.fill(0xff);

    for (const auto sprite : valid_sprites) {
      auto top = sprite->y - 16;
      auto row = sprite->attrs.y_flip ? height - (y - top) - 1  : y - top;
      uint8_t tile_id = sprite->tile;
      if (regs_.lcdc.sprite_size) {
        if (row < 8) {
          tile_id &= 0xfe;
        } else {
          tile_id |= 0x01;
        }
      }
      auto tile_idx = (addr_with_mode(1, tile_id) - kVRAMAddrStart) / 16;
      auto tile = vram_.tile_data[tile_idx];
      auto palette = sprite->attrs.dmg_palette ? regs_.obp1: regs_.obp0;
      auto left = sprite->x - 8;

      for (auto x = sprite->x - 8; x < sprite->x; x += 1) {
        if (x < 0 || x >= kLCDWidth) {
          continue;
        }

        if (sprite->attrs.priority && bg_win_pixels[x] != 0) {
          continue;
        }

        if (sprite_prio[x] <= sprite->x) {
          continue;
        }

        auto xi = sprite->attrs.x_flip ? x - left : 7 - (x - left);
        uint16_t hi = (tile[row % 8] >> 8) >> xi;
        uint8_t lo = tile[row % 8] >> xi;
        uint8_t bits = ((hi & 0b1) << 1) | (lo & 0b1);

        if (bits) {
        auto cid = get_palette_index(bits, palette);
          ImageDrawPixel(&target_lcd_back_, x, y, kLCDPalette[cid]);
          sprite_prio[x] = sprite->x;
        }
      }
    }
  }
}

const Texture2D& Ppu::GetTextureLcd() const {
  return target_lcd_front_;
}

const RenderTexture2D& Ppu::GetTextureTilemap1() const {
  return target_tilemap1_;
}

const RenderTexture2D& Ppu::GetTextureTilemap2() const {
  return target_tilemap2_;
}

const RenderTexture2D& Ppu::GetTextureSprites() const {
  return target_sprites_;
}

const RenderTexture2D& Ppu::GetTextureTiles() const {
  return target_tiles_;
}

void Ppu::UpdateRenderTargets() {
  constexpr int tile_width = 16;
  constexpr int tile_height = 24;

  BeginTextureMode(target_tiles_);
  {
    int x = 0;
    int y = 0;
    for (auto &tile : vram_.tile_data) {
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

  BeginTextureMode(target_tilemap1_);
  {
    auto tiledata_area = regs_.lcdc.tiledata_area;
    auto &tilemap = vram_.tile_map[0];

    int x = 0;
    int y = 0;
    for (auto tile_id : tilemap) {
      auto tile_idx = (addr_with_mode(tiledata_area, tile_id) - kVRAMAddrStart) / 16;
      auto dst_y = tile_idx / 16;
      auto dst_x = tile_idx % 16;

      Rectangle rect {
        static_cast<float>(dst_x * 8),
        static_cast<float>(target_tiles_.texture.height - (dst_y * 8) - 8),
        8.f,
        -8.f,
      };

      Vector2 pos {
        static_cast<float>(x * 8),
        static_cast<float>(y * 8),
      };

      DrawTextureRec(target_tiles_.texture, rect, pos, WHITE);

      x += 1;
      if (x >= 32) {
        x = 0;
        y += 1;
      }
    }

    if (regs_.lcdc.bg_tilemap_area == 0) {
      auto x1 = regs_.scx;
      auto y1 = regs_.scy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, RED);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, RED);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, RED);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, RED);
    }
    if (regs_.lcdc.window_tilemap_area == 0) {
      auto x1 = regs_.wx < 7 ? kLCDWidth + regs_.wx - 7 : regs_.wx - 7;
      auto y1 = regs_.wy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, BLUE);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, BLUE);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, BLUE);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, BLUE);
    }
  }
  EndTextureMode();

  BeginTextureMode(target_tilemap2_);
  {
    auto tiledata_area = regs_.lcdc.tiledata_area;
    auto &tilemap = vram_.tile_map[1];

    int x = 0;
    int y = 0;
    for (auto tile_id : tilemap) {
      auto tile_idx = (addr_with_mode(tiledata_area, tile_id) - kVRAMAddrStart) / 16;
      auto dst_y = tile_idx / 16;
      auto dst_x = tile_idx % 16;

      Rectangle rect {
        static_cast<float>(dst_x * 8),
        static_cast<float>(target_tiles_.texture.height - (dst_y * 8) - 8),
        8.f,
        -8.f,
      };

      Vector2 pos {
        static_cast<float>(x * 8),
        static_cast<float>(y * 8),
      };

      DrawTextureRec(target_tiles_.texture, rect, pos, WHITE);

      x += 1;
      if (x >= 32) {
        x = 0;
        y += 1;
      }
    }

    if (regs_.lcdc.bg_tilemap_area == 1) {
      auto x1 = regs_.scx;
      auto y1 = regs_.scy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, RED);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, RED);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, RED);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, RED);
    }
    if (regs_.lcdc.window_tilemap_area == 1) {
      auto x1 = regs_.wx < 7 ? kLCDWidth + (regs_.wx - 7) : regs_.wx - 7;
      auto y1 = regs_.wy;
      auto x2 = (x1 + kLCDWidth - 1) % 256;
      auto y2 = (y1 + kLCDHeight - 1) % 256;

      DrawLine(x1, y1, x2 < x1 ? 255 : x2, y1, BLUE);
      DrawLine(x1, y2, x2 < x1 ? 255 : x2, y2, BLUE);
      DrawLine(x1, y1, x1, y2 < y1 ? 255 : y2, BLUE);
      DrawLine(x2, y1, x2, y2 < y1 ? 255 : y2, BLUE);
    }
  }
  EndTextureMode();

  BeginTextureMode(target_sprites_);
  {
    ClearBackground(BLANK);

    auto sprite_tile_height = regs_.lcdc.sprite_size ? 2 : 1;
    auto row = 0;
    auto col = 0;

    for (auto &sprite : oam_.sprites) {
      for (auto ti = 0; ti < sprite_tile_height; ti += 1) {
        auto tile_idx = ((addr_with_mode(1, sprite.tile) - kVRAMAddrStart) / 16) + ti;
        auto dst_y = tile_idx / 16;
        auto dst_x = tile_idx % 16;

        Rectangle rect {
          static_cast<float>(dst_x * 8),
          static_cast<float>(target_tiles_.texture.height - (dst_y * 8) - 8),
          8.f,
          -8.f,
        };

        Vector2 pos {
          static_cast<float>(col * 8),
          static_cast<float>((row * sprite_tile_height * 8) + (ti * 8))
        };

        DrawTextureRec(target_tiles_.texture, rect, pos, WHITE);

        col += 1;
        if (col >= 8) {
          col = 0;
          row += 1;
        }
      }
    }
  }
  EndTextureMode();
}

bool Ppu::IsValidFor(uint16_t addr) const {
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

void Ppu::Write8(uint16_t addr, uint8_t byte) {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    vram_.bytes[addr - kVRAMAddrStart] = byte;
    return;
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    oam_.bytes[addr - kOAMAddrStart] = byte;
    return;
  }

  if (addr == std::to_underlying(IO::LY)) {
    return;
  }

  if (addr == std::to_underlying(IO::STAT)) {
    regs_.stat.val = (regs_.stat.val & 0b11) | (byte & ~0b11);
    return;
  }

  regs_.bytes[addr - std::to_underlying(IO::LCDC)] = byte;

  if (addr == std::to_underlying(IO::LCDC) && !regs_.lcdc.lcd_enable) {
    regs_.ly = 0;
    cycle_counter_ = 0;
    window_line_counter_ = 0;
    regs_.stat.ppu_mode = 0;
    ClearTargetBuffers();
  } else if (addr == std::to_underlying(IO::LYC)) {
    regs_.stat.coincidence_flag = regs_.lyc == regs_.ly;
    if (regs_.stat.coincidence_flag && regs_.stat.stat_interrupt_lyc) {
      interrupts_.RequestInterrupt(Interrupt::Stat);
    }
  } else if (addr == std::to_underlying(IO::DMA)) {
    StartDma();
  }
}

uint8_t Ppu::Read8(uint16_t addr) const {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return vram_.bytes[addr - kVRAMAddrStart];
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    return oam_.bytes[addr - kOAMAddrStart];
  }

  if (addr == std::to_underlying(IO::STAT)) {
    return regs_.stat.val | 0b10000000;
  }

  if (log_doctor_ && addr == std::to_underlying(IO::LY)) {
    return 0x90;
  }

  return regs_.bytes[addr - std::to_underlying(IO::LCDC)];
}

void Ppu::Reset() {
  vram_.reset();
  oam_.reset();
  regs_.reset();
  cycle_counter_ = 0;
  window_line_counter_ = 0;

  BeginTextureMode(target_tiles_);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_tilemap1_);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_tilemap2_);
  ClearBackground(BLACK);
  EndTextureMode();

  BeginTextureMode(target_sprites_);
  ClearBackground(BLACK);
  EndTextureMode();

  ClearTargetBuffers();
}

void Ppu::ClearTargetBuffers() {
  ImageClearBackground(&target_lcd_back_, BLACK);
  UpdateTexture(target_lcd_front_, target_lcd_back_.data);
}

PPUMode Ppu::GetMode() const {
  return static_cast<PPUMode>(regs_.stat.ppu_mode);
}

void Ppu::SetMode(PPUMode mode) {
  regs_.stat.ppu_mode = std::to_underlying(mode);
}

void Ppu::StartDma() {
  auto source = regs_.dma << 8;

  if (source >= kVRAMAddrStart && source <= kVRAMAddrEnd) {
    auto base = source - kVRAMAddrStart;
    for (auto i = 0; i < oam_.bytes.size(); i += 1) {
      oam_.bytes[i] = vram_.bytes[base + i];
    }
    return;
  }

  if (source >= kExtRamBusEnd) {
    source = kExtRamBusStart + (source & kExtRamBusMask);
  }

  for (auto i = 0; i < oam_.bytes.size(); i += 1) {
    oam_.bytes[i] = mmu_.Read8(source + i);
  }
}
