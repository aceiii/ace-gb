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

void PPU::execute(uint8_t cycles) {
  do {
    step();
  } while(--cycles);
}

inline void PPU::step() {
  if (!enable_lcd) {
    return;
  }

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
        scanline_y += 1;
        PPUMode new_mode;
        if (scanline_y >= kLCDHeight) {
          new_mode = PPUMode::VBlank;

          // TODO: enable vblank interrupt
//          uint8_t interrupt = mmu->read8(std::to_underlying(IO::IF));
//          uint8_t mask = 1 << std::to_underlying(Interrupt::VBlank);
//          interrupt = (interrupt & ~mask) | (1 << std::to_underlying(Interrupt::VBlank));
//          mmu->write(std::to_underlying(IO::IF), interrupt);

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
        scanline_y += 1;

        if (scanline_y >= kLCDHeight + 10) {
          scanline_y = 0;
          mode = PPUMode::OAM;
        }
      }
      break;
  }
}

const RenderTexture2D* PPU::target() const {
  return &targets[target_index];
}

void PPU::populate_sprite_buffer() {
  if (!enable_lcd) {
    return;
  }
}

void PPU::write8(uint16_t addr, uint8_t byte) {
}

void PPU::write16(uint16_t addr, uint16_t word) {
}

[[nodiscard]] uint8_t PPU::read8(uint16_t addr) const {
}

[[nodiscard]] uint16_t PPU::read16(uint16_t addr) const {
}

void PPU::reset() {
}

PPUMode PPU::current_mode() const {
  return mode;
}
