#include <utility>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

#include "io.hpp"
#include "ppu.hpp"


namespace {
  constexpr u16 kLCDWidth = 160;
  constexpr u16 kLCDHeight = 144;

  constexpr u16 kVRAMAddrStart = 0x8000;
  constexpr u16 kVRAMAddrEnd = 0x9FFF;
  constexpr u16 kVRAMRelStart = 0x9000;

  constexpr u16 kOAMAddrStart = 0xFE00;
  constexpr u16 kOAMAddrEnd = 0xFE9F;

  constexpr u16 kExtRamBusStart = 0xA000;
  constexpr u16 kExtRamBusEnd = 0xDFFF;
  constexpr u16 kExtRamBusMask = kExtRamBusEnd - kExtRamBusStart;

  constexpr size_t kDotsPerOAM = 80;
  constexpr size_t kDotsPerDraw = 172;
  constexpr size_t kDotsPerRow = 456;
}

inline u8 GetPaletteIndex(u8 id, u8 palette) {
  return (palette >> (2 * id)) & 0b11;
}

inline u16 AddrMode8000(u8 addr) {
  return kVRAMAddrStart + addr * 16;
}

inline u16 AddrMode8800(u8 addr) {
  return kVRAMRelStart + (static_cast<i8>(addr) * 16);
}

inline u16 AddrWithMode(u8 mode, u8 addr) {
  return mode ? AddrMode8000(addr) : AddrMode8800(addr);
}

void Ppu::Init(PpuConfig cfg) {
  mmu_ = cfg.mmu;
  state_ = cfg.state;
  interrupts_ = cfg.interrupts;

  auto logger = spdlog::get("doctor_logger");
  log_doctor_ = logger != nullptr;

  target_lcd_back_ = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  target_lcd_front_ = LoadTextureFromImage(target_lcd_back_);

  constexpr int tiles_width = 16 * 8;
  constexpr int tiles_height = 24 * 8;
  target_tiles_ = LoadRenderTexture(tiles_width, tiles_height);

  constexpr int palettes_width = 136;
  constexpr int palettes_height = 128;
  target_palettes_ = LoadRenderTexture(palettes_width, palettes_height);

  constexpr int tilemap_width = 256;
  constexpr int tilemap_height = 256;
  target_tilemap1_ = LoadRenderTexture(tilemap_width, tilemap_height);
  target_tilemap2_ = LoadRenderTexture(tilemap_width, tilemap_height);

  constexpr int sprites_width = 8 * 8;
  constexpr int sprites_height = 5 * 16;
  target_sprites_ = LoadRenderTexture(sprites_width, sprites_height);

  target_lcd_back_ = GenImageColor(kLCDWidth, kLCDHeight, BLACK);
  ImageFormat(&target_lcd_back_, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  palette_ = std::move(cfg.palette);
}

void Ppu::Cleanup() {
  UnloadRenderTexture(target_tiles_);
  UnloadRenderTexture(target_tilemap1_);
  UnloadRenderTexture(target_tilemap2_);
  UnloadRenderTexture(target_sprites_);
  UnloadRenderTexture(target_palettes_);

  UnloadImage(target_lcd_back_);
  UnloadTexture(target_lcd_front_);
}

void Ppu::OnTick() {
  ZoneScoped;
  Step(0);
  Step(1);
  Step(2);
  Step(3);
}

inline void Ppu::Step(int n) {
  ZoneScoped;

  if (n % 2 == 0 && state_->halt && dma_state_.active && !dma_state_.hdma) {
    mmu_->Write8(dma_state_.destination++, mmu_->Read8(dma_state_.source++));
    dma_state_.length--;
    if (!dma_state_.length) {
      state_->halt = false;
      dma_state_.active = false;
    }
  }

  if (!regs_.lcdc.lcd_enable) {
    return;
  }

  auto mode = this->GetMode();

  static u8 hblank_dma_counter = 0;
  if (n % 2 == 0 && mode == PPUMode::HBlank && !state_->halt && hblank_dma_counter) {
    mmu_->Write8(dma_state_.destination++, mmu_->Read8(dma_state_.source++));
    hblank_dma_counter--;
    dma_state_.length--;
    if (!dma_state_.length) {
      dma_state_.active = false;
    }
  }

  if (++cycle_counter_ >= kDotsPerRow) {
    regs_.ly = (regs_.ly + 1) % (kLCDHeight + 10);
    regs_.stat.coincidence_flag = regs_.ly == regs_.lyc;
    if (regs_.stat.coincidence_flag && regs_.stat.stat_interrupt_lyc) {
      interrupts_->RequestInterrupt(Interrupt::Stat);
    }

    cycle_counter_ = 0;
  }

  if (regs_.ly >= kLCDHeight) {
    if (mode != PPUMode::VBlank) {
      window_line_counter_ = 0;
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::VBlank);
      interrupts_->RequestInterrupt(Interrupt::VBlank);
      if (regs_.stat.stat_interrupt_mode0) {
        interrupts_->RequestInterrupt(Interrupt::Stat);
      }
      SwapLcdTargets();
    }
  } else if (cycle_counter_ < kDotsPerOAM) {
    if (mode != PPUMode::OAM) {
      regs_.stat.ppu_mode = std::to_underlying(PPUMode::OAM);
      if (regs_.stat.stat_interrupt_mode2) {
        interrupts_->RequestInterrupt(Interrupt::Stat);
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
        interrupts_->RequestInterrupt(Interrupt::Stat);
      }
      if (dma_state_.active && dma_state_.hdma && dma_state_.length) {
        hblank_dma_counter = static_cast<u8>(std::clamp<u16>(dma_state_.length, 0, 0x10));
      }
      DrawLcdRow();
    }
  }
}

void Ppu::SwapLcdTargets() {
  frame_count_ += 1;
  UpdateTexture(target_lcd_front_, target_lcd_back_.data);
}

void Ppu::DrawLcdRow() {
  ZoneScoped;

  struct BgPixels {
    u8 priority;
    u8 bits;
  };

  static std::array<BgPixels, kLCDWidth> bg_win_pixels;
  bg_win_pixels.fill({});

  bool enable_bg = hardware_mode_ == HardwareMode::kDmgMode ? regs_.lcdc.bg_window_enable : true;
  bool bg_low_priority = hardware_mode_ == HardwareMode::kDmgMode ? false : !regs_.lcdc.bg_window_enable;

  if (enable_bg) {
    const bool enable_window_flag = regs_.lcdc.window_enable &&
      regs_.wx >= 0 && regs_.wx <= 166 && regs_.wy >= 0 && regs_.wy <= 143;
    const u8 y = regs_.ly;

    u8 py = regs_.scy + y;
    u8 ty = (py >> 3 ) & 31;
    u8 row = py % 8;

    bool enable_window = false;
    for (auto x = 0; x < kLCDWidth; x += 1) {
      if (!enable_window && enable_window_flag && x >= regs_.wx - 7 && y >= regs_.wy) {
        enable_window = true;
        py = window_line_counter_;// y - regs.wy;
        ty = (py >> 3 ) & 31;
        row = py % 8;
      }

      auto tilemap_idx = enable_window ? regs_.lcdc.window_tilemap_area : regs_.lcdc.bg_tilemap_area;
      auto& tilemap = BankAt(0).tile_map[tilemap_idx];
      auto& attrmap = BankAt(1).tile_map[tilemap_idx];

      u8 px = enable_window ? x - (regs_.wx - 7) : regs_.scx + x;
      u8 tx = (px >> 3) & 31;
      u8 sub_x = px % 8;

      auto map_idx = (ty * 32) + tx;
      auto tile_id = tilemap[map_idx].tile_id;
      auto tile_attr = attrmap[map_idx];
      auto tile_idx = (AddrWithMode(regs_.lcdc.tiledata_area, tile_id) - kVRAMAddrStart) / 16;
      auto tile = BankAt(tile_attr.bank).tile_data[tile_idx];

      auto xi = tile_attr.x_flip ? sub_x : 7 - sub_x;
      auto actual_row = tile_attr.y_flip ? 7 - row : row;

      u16 hi = (tile[actual_row] >> 8) >> xi;
      u8 lo = tile[actual_row] >> xi;
      u8 bits = ((hi & 0b1) << 1) | (lo & 0b1);

      if (hardware_mode_ == HardwareMode::kDmgMode) {
        auto cid = GetPaletteIndex(bits, regs_.bgp);
        auto color = palette_[cid];
        ImageDrawPixel(&target_lcd_back_, x, y, color);
      } else {
        bg_win_pixels[x].priority = tile_attr.priority;
        auto cgb_palette = cgb_bg_palettes_[tile_attr.palette];
        ImageDrawPixel(&target_lcd_back_, x, y, cgb_palette[bits & 0b11]);
      }

      bg_win_pixels[x].bits = bits;
    }

    if (enable_window) {
      window_line_counter_++;
    }
  } else {
    if (hardware_mode_ == HardwareMode::kDmgMode) {
      auto cid = GetPaletteIndex(0, regs_.bgp);
      auto color = palette_[cid];
      ImageDrawLine(&target_lcd_back_, 0, regs_.ly, kLCDWidth, regs_.ly,  color);
    } else {
      ImageDrawLine(&target_lcd_back_, 0, regs_.ly, kLCDWidth, regs_.ly,  cgb_bg_palettes_[0][0]);
    }
  }

  if (regs_.lcdc.sprite_enable) {
    static std::vector<Sprite*> valid_sprites;
    valid_sprites.clear();
    valid_sprites.reserve(10);

    const auto height = regs_.lcdc.sprite_size ? 16 : 8;
    const auto y = regs_.ly;

    for (auto& sprite : oam_.sprites) {
      auto top = sprite.y - 16;
      auto bottom = top + height;
      if (y >= top && y < bottom) {
        valid_sprites.push_back(&sprite);
        if (valid_sprites.size() >= 10) {
          break;
        }
      }
    }

    static std::array<u8, kLCDWidth> sprite_prio;
    sprite_prio.fill(0xff);

    u8 oam_idx = 0;
    for (const auto sprite : valid_sprites) {
      auto top = sprite->y - 16;
      auto row = sprite->attrs.y_flip ? height - (y - top) - 1  : y - top;
      u8 tile_id = sprite->tile;
      if (regs_.lcdc.sprite_size) {
        if (row < 8) {
          tile_id &= 0xfe;
        } else {
          tile_id |= 0x01;
        }
      }
      auto tile_idx = (AddrWithMode(1, tile_id) - kVRAMAddrStart) / 16;
      auto tile = BankAt(sprite->attrs.cgb_bank).tile_data[tile_idx];
      auto left = sprite->x - 8;

      for (auto x = sprite->x - 8; x < sprite->x; x += 1) {
        if (x < 0 || x >= kLCDWidth) {
          continue;
        }

        bool draw_sprite = bg_low_priority || bg_win_pixels[x].bits == 0 || (!sprite->attrs.priority && !bg_win_pixels[x].priority);
        if (!draw_sprite) {
          continue;
        }

        if (sprite_prio[x] <= sprite->x) {
          continue;
        }

        auto xi = sprite->attrs.x_flip ? x - left : 7 - (x - left);
        u16 hi = (tile[row % 8] >> 8) >> xi;
        u8 lo = tile[row % 8] >> xi;
        u8 bits = ((hi & 0b1) << 1) | (lo & 0b1);

        if (bits) {
          if (hardware_mode_ == HardwareMode::kDmgMode) {
            auto palette = sprite->attrs.dmg_palette ? regs_.obp1: regs_.obp0;
            auto cid = GetPaletteIndex(bits, palette);
            ImageDrawPixel(&target_lcd_back_, x, y, palette_[cid]);
            sprite_prio[x] = sprite->x;
          } else {
            auto cgb_palette = cgb_sprite_palettes_[sprite->attrs.cgb_palette];
            ImageDrawPixel(&target_lcd_back_, x, y, cgb_palette[bits & 0b11]);
            sprite_prio[x] = oam_idx;
          }
        }
      }
      oam_idx += 1;
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

const RenderTexture2D& Ppu::GetTexturePalettes() const {
  return target_palettes_;
}

void Ppu::UpdateRenderTargets() {
  ZoneScoped;

  constexpr int tile_width = 16;
  constexpr int tile_height = 24;

  BeginTextureMode(target_tiles_);
  {
    ZoneScopedN("BeginTextureMode:target_tiles");

    const auto& vram = BankAt(0);

    int x = 0;
    int y = 0;

    for (auto& tile : vram.tile_data) {
      for (int row = 0; row < tile.size(); row += 1) {
        u16 hi = (tile[row] >> 8) << 1;
        u8 lo = tile[row];
        for (int b = 7; b >= 0; b -= 1) {
          u8 bits = (hi & 0b10) | (lo & 0b1);
          auto color = palette_[bits];
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
    ZoneScopedN("BeginTextureMode:target_tilemap1");

    auto tiledata_area = regs_.lcdc.tiledata_area;
    auto& tilemap = BankAt(0).tile_map[0];

    int x = 0;
    int y = 0;
    for (auto tile : tilemap) {
      auto tile_idx = (AddrWithMode(tiledata_area, tile.tile_id) - kVRAMAddrStart) / 16;
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
    ZoneScopedN("BeginTextureMode:target_tilemap2");

    auto tiledata_area = regs_.lcdc.tiledata_area;
    auto& tilemap = BankAt(0).tile_map[1];

    int x = 0;
    int y = 0;
    for (auto tile : tilemap) {
      auto tile_idx = (AddrWithMode(tiledata_area, tile.tile_id) - kVRAMAddrStart) / 16;
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
    ZoneScopedN("BeginTextureMode:target_sprites");

    ClearBackground(BLANK);

    auto sprite_tile_height = regs_.lcdc.sprite_size ? 2 : 1;
    auto row = 0;
    auto col = 0;

    for (auto& sprite : oam_.sprites) {
      for (auto ti = 0; ti < sprite_tile_height; ti += 1) {
        auto tile_idx = ((AddrWithMode(1, sprite.tile) - kVRAMAddrStart) / 16) + ti;
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

  BeginTextureMode(target_palettes_);
  {
    ZoneScopedN("BeginTextureMode:target_palettes");

    ClearBackground(BLANK);

    const int w = 8;
    const int h = 8;

    DrawText("BG", 4, 4, 10, RED);
    for (auto i = 0; i < kCgbNumPalettes; i++) {
      int x = i * w;
      int y = 0;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_bg_palettes_[i][0]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_bg_palettes_[i][1]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_bg_palettes_[i][2]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_bg_palettes_[i][3]);
    }

    DrawText("Sprite", 4, 50, 10, RED);
      for (auto i = 0; i < kCgbNumPalettes; i++) {
      int x = i * w;
      int y = 64;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_sprite_palettes_[i][0]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_sprite_palettes_[i][1]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_sprite_palettes_[i][2]);
      y += h;
      DrawRectangle(4 + x, 14 + y, w, h, cgb_sprite_palettes_[i][3]);
    }
  }
  EndTextureMode();
}

bool Ppu::IsValidFor(u16 addr) const {
  if (addr == std::to_underlying(IO::VBK)) {
    return true;
  }

  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return true;
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    return true;
  }

  if (addr >= std::to_underlying(IO::LCDC) && addr <= std::to_underlying(IO::WX)) {
    return true;
  }

  if (addr >= std::to_underlying(IO::HDMA1) && addr <= std::to_underlying(IO::HDMA5)) {
    return true;
  }

  if (addr == std::to_underlying(IO::BCPS) || addr == std::to_underlying(IO::BCPD) || addr == std::to_underlying(IO::OCPS) || addr == std::to_underlying(IO::OCPD)) {
    return true;
  }

  return false;
}

void Ppu::Write8(u16 addr, u8 byte) {
  if (addr == std::to_underlying(IO::VBK)) {
    vbk_.val = byte;
    return;
  }

  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    Bank().bytes[addr - kVRAMAddrStart] = byte;
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

  if (addr == std::to_underlying(IO::HDMA1)) {
    dma_regs_.source.high = byte;
    return;
  }

  if (addr == std::to_underlying(IO::HDMA2)) {
    dma_regs_.source.low = byte;
    return;
  }

  if (addr == std::to_underlying(IO::HDMA3)) {
    dma_regs_.destination.high = byte;
    return;
  }

  if (addr == std::to_underlying(IO::HDMA4)) {
    dma_regs_.destination.low = byte;
    return;
  }

  if (addr == std::to_underlying(IO::HDMA5)) {
    const auto dma_type = (byte >> 7) & 0b1;
    dma_regs_.dma.mode_active = byte;
    if (dma_type) {
      StartHBlankDma();
    } else {
      StartGPDma();
    }
    return;
  }

  if (addr == std::to_underlying(IO::BCPS)) {
    cgb_regs_.bcps.val = byte;
    return;
  }

  if (addr == std::to_underlying(IO::BCPD)) {
    auto& col = cgb_regs_.bcpd[cgb_regs_.bcps.address >> 1];
    if (cgb_regs_.bcps.address & 0x1) {
      col.hi = byte;
    } else {
      col.lo = byte;
    }

    auto palette_idx = (cgb_regs_.bcps.address >> 3) & 0x7;
    auto color_idx = (cgb_regs_.bcps.address >> 1) & 0x3;
    cgb_bg_palettes_[palette_idx][color_idx] = col.GetColor();

    if (cgb_regs_.bcps.auto_increment) {
      cgb_regs_.bcps.address++;
    }
    return;
  }

  if (addr == std::to_underlying(IO::OCPS)) {
    cgb_regs_.ocps.val = byte;
    return;
  }

  if (addr == std::to_underlying(IO::OCPD)) {
    auto& col = cgb_regs_.ocpd[cgb_regs_.ocps.address >> 1];
    if (cgb_regs_.ocps.address & 0x1) {
      col.hi = byte;
    } else {
      col.lo = byte;
    }

    auto palette_idx = (cgb_regs_.ocps.address >> 3) & 0x7;
    auto color_idx = (cgb_regs_.ocps.address >> 1) & 0x3;
    cgb_sprite_palettes_[palette_idx][color_idx] = col.GetColor();

    if (cgb_regs_.ocps.auto_increment) {
      cgb_regs_.ocps.address++;
    }
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
      interrupts_->RequestInterrupt(Interrupt::Stat);
    }
  } else if (addr == std::to_underlying(IO::DMA)) {
    StartDma();
  }
}

u8 Ppu::Read8(u16 addr) const {
  if (addr == std::to_underlying(IO::VBK)) {
    return vbk_.val;
  }

  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return Bank().bytes[addr - kVRAMAddrStart];
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

  if (addr == std::to_underlying(IO::HDMA1)) {
    return dma_regs_.source.high;
  }

  if (addr == std::to_underlying(IO::HDMA2)) {
    return dma_regs_.source.low;
  }

  if (addr == std::to_underlying(IO::HDMA3)) {
    return dma_regs_.destination.high;
  }

  if (addr == std::to_underlying(IO::HDMA4)) {
    return dma_regs_.destination.low;
  }

  if (addr == std::to_underlying(IO::HDMA5)) {
    if (dma_state_.active) {
      return ((dma_state_.length / 0x10) - 1) & 0x7f;
    }
    return 0xff;
  }

  if (addr == std::to_underlying(IO::BCPS)) {
    return cgb_regs_.bcps.val;
  }

  if (addr == std::to_underlying(IO::BCPD)) {
    const auto& col = cgb_regs_.bcpd[cgb_regs_.bcps.address >> 1];
    if (cgb_regs_.bcps.address & 0x01) {
      return col.lo;
    } else {
      return col.hi;
    }
  }

  if (addr == std::to_underlying(IO::OCPS)) {
    return cgb_regs_.ocps.val;
  }

  if (addr == std::to_underlying(IO::OCPD)) {
    const auto& col = cgb_regs_.ocpd[cgb_regs_.ocps.address >> 1];
    if (cgb_regs_.ocps.address & 0x01) {
      return col.lo;
    } else {
      return col.hi;
    }
  }

  return regs_.bytes[addr - std::to_underlying(IO::LCDC)];
}

void Ppu::Reset() {
  for (auto& bank : banks_) {
    bank.reset();
  }
  oam_.reset();
  regs_.reset();
  cycle_counter_ = 0;
  window_line_counter_ = 0;
  frame_count_ = 0;

  // hardware_mode_ = HardwareMode::kDmgMode;
  // banks_ = {};
  // cgb_bg_palettes_ = {};
  // cgb_sprite_palettes_ = {};
  // oam_ = {};
  // regs_ = {};
  // vbk_ = {};
  // palette_ = {};
  // dma_regs_ = {};
  // dma_state_ = {};
  // cgb_regs_ = {};

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

  BeginTextureMode(target_palettes_);
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
      oam_.bytes[i] = Bank().bytes[base + i];
    }
    return;
  }

  if (source >= kExtRamBusEnd) {
    source = kExtRamBusStart + (source & kExtRamBusMask);
  }

  for (auto i = 0; i < oam_.bytes.size(); i += 1) {
    oam_.bytes[i] = mmu_->Read8(source + i);
  }
}

void Ppu::ResetFrameCount() {
  frame_count_ = 0;
}

size_t Ppu::GetFrameCount() const {
  return frame_count_;
}

void Ppu::UpdatePalette(std::array<Color, 4> palette) {
  palette_ = std::move(palette);
}

VramMemory& Ppu::Bank() {
  return banks_.at(vbk_.bank);
}

const VramMemory& Ppu::Bank() const {
  return banks_.at(vbk_.bank);
}

const VramMemory& Ppu::BankAt(u8 bit) const {
  return banks_.at(bit & 0b1);
}

void Ppu::StartGPDma() {
  state_->halt = true;
  dma_state_ = {
    .active = true,
    .hdma = false,
    .length = static_cast<u16>((dma_regs_.dma.length + 1) * 0x10),
    .source = static_cast<u16>(dma_regs_.source.val & 0xfff0),
    .destination = static_cast<u16>(0x8000 + (dma_regs_.destination.val & 0x1ff0)),
  };
  spdlog::debug("GP DMA triggered: src={:04x}, dst={:04x}, len={:02x}", dma_state_.source, dma_state_.destination, dma_state_.length);
}

void Ppu::StartHBlankDma() {
  dma_state_ = {
    .active = true,
    .hdma = true,
    .length = static_cast<u16>((dma_regs_.dma.length + 1) * 0x10),
    .source = static_cast<u16>(dma_regs_.source.val & 0xfff0),
    .destination = static_cast<u16>(0x8000 + (dma_regs_.destination.val & 0x1ff0)),
  };
  spdlog::debug("HBlank DMA triggered: src={:04x}, dst={:04x}, len={:02x}", dma_state_.source, dma_state_.destination, dma_state_.length);
}

void Ppu::SetHardwareMode(HardwareMode mode) {
  hardware_mode_ = mode;
}

HardwareMode Ppu::GetHardwareMode() const {
  return hardware_mode_;
}
