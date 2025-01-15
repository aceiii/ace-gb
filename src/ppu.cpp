#include <cstdint>
#include <utility>
#include <spdlog/spdlog.h>

#include "ppu.h"
#include "cpu.h"

const int kLCDWidth = 160;
const int kLCDHeight = 144;

const std::array<Color, 4> kLCDPalette {
  Color{240, 240, 240, 255},
  Color{178, 178, 178, 255},
  Color{100, 100, 100, 255},
  Color {48, 48, 48, 255},
};

void PPU::init() {
  for (int i = 0; i < targets.size(); i += 1) {
    targets[i] = LoadRenderTexture(kLCDWidth, kLCDHeight);
    BeginTextureMode(targets[i]);
    ClearBackground(Color(75, 0, 1, 255));
    EndTextureMode();
  }
}

void PPU::cleanup() {
  for (int i = 0; i < targets.size(); i += 1) {
    UnloadRenderTexture(targets[i]);
  }
}

void PPU::reset() {
}

void PPU::execute(IMMU *mmu, uint8_t cycles) {
  do {
    step(mmu);
  } while(--cycles);
}

inline void PPU::step(IMMU *mmu) {
  auto lcd_control = mmu->read8(std::to_underlying(IO::LCDC));
  if (!(lcd_control & std::to_underlying(LCDControlMask::LCDDisplayEnable))) {
    return;
  }

  auto mode = current_mode(mmu);
  if (mode == PPUMode::OAM && cycle_counter == 0) {
    populate_sprite_buffer(mmu);
  } else if (mode == PPUMode::Draw && cycle_counter == 80) {
    
  }

  cycle_counter += 1;

  switch (mode) {
    case PPUMode::OAM:
      if (cycle_counter >= 80) {
        set_mode(mmu, PPUMode::Draw);
      }
      break;
    case PPUMode::Draw: break;
    case PPUMode::HBlank:
      if (cycle_counter >= 456) {
        cycle_counter = 0;
        scanline_y += 1;
        PPUMode new_mode;
        if (scanline_y >= kLCDHeight) {
          new_mode = PPUMode::VBlank;

          uint8_t interrupt = mmu->read8(std::to_underlying(IO::IF));
          uint8_t mask = 1 << std::to_underlying(Interrupt::VBlank);
          interrupt = (interrupt & ~mask) | (1 << std::to_underlying(Interrupt::VBlank));
          mmu->write(std::to_underlying(IO::IF), interrupt);

          target_index = target_index % targets.size();
        } else {
          new_mode = PPUMode::OAM;
        }
        mmu->write(std::to_underlying(IO::LY), scanline_y);
        set_mode(mmu, new_mode);
      }
      break;
    case PPUMode::VBlank:
      if (cycle_counter >= 456) {
        cycle_counter = 0;
        scanline_y += 1;

        if (scanline_y >= kLCDHeight + 10) {
          scanline_y = 0;
          set_mode(mmu, PPUMode::OAM);
        }

        mmu->write(std::to_underlying(IO::LY), static_cast<uint8_t>(scanline_y));
      }
      break;
  }
}

const RenderTexture2D* PPU::target() const {
  return &targets[target_index];
}

PPUMode PPU::current_mode(IMMU *mmu) const {
  auto lcd_status = mmu->read8(std::to_underlying(IO::STAT));
  PPUMode mode {lcd_status & std::to_underlying(LCDStatusMask::PPUMode)};
  return mode;
}

void PPU::set_mode(IMMU *mmu, PPUMode mode) {
  auto lcd_status = mmu->read8(std::to_underlying(IO::STAT));
  lcd_status = lcd_status & ~std::to_underlying(LCDStatusMask::PPUMode) | std::to_underlying(PPUMode::OAM);
  mmu->write(std::to_underlying(IO::STAT), lcd_status);
}

void PPU::populate_sprite_buffer(IMMU *mmu) {
  num_sprites = 0;

  uint8_t sprite_height = 8;

  auto lcdc = mmu->read8(std::to_underlying(IO::LCDC));
  if (lcdc & std::to_underlying(LCDControlMask::SpriteSize)) {
    sprite_height = 16;
  }

  for (uint16_t base = 0xfe00; base < 0xfea0 && num_sprites < sprites.size(); base += 4) {
    uint8_t y = mmu->read8(base);
    uint8_t x = mmu->read8(base + 1);

    if (x > 0 && (scanline_y + 16) >= y && scanline_y + 16 < (y + sprite_height)) {
      sprites[num_sprites++] = base;
    }
  }
}
