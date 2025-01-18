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
        regs.lyc += 1;
        PPUMode new_mode;
        if (regs.lyc >= kLCDHeight) {
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
        regs.lyc += 1;

        if (regs.lyc >= kLCDHeight + 10) {
          regs.lyc = 0;
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
  if (!regs.lcdc.lcd_display_enable) {
    return;
  }
}

void PPU::write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::LCDC):
       regs.lcdc.val = byte;
       break;
    case std::to_underlying(IO::STAT):
      regs.stat.val = byte;
      break;
    case std::to_underlying(IO::SCY):
      regs.scy = byte;
      break;
    case std::to_underlying(IO::SCX):
      regs.scx = byte;
      break;
    case std::to_underlying(IO::LY):
      regs.ly = byte;
      break;
    case std::to_underlying(IO::LYC):
      regs.lyc = byte;
      break;
    default:
      break;
  }
}

[[nodiscard]] uint8_t PPU::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::LCDC):
      return regs.lcdc.val;
    case std::to_underlying(IO::STAT):
      return regs.stat.val;
    case std::to_underlying(IO::SCY):
      return regs.scy;
    case std::to_underlying(IO::SCX):
      return regs.scx;
    case std::to_underlying(IO::LY):
      return regs.ly;
    case std::to_underlying(IO::LYC):
      return regs.lyc;
    default:
      return 0;
  }
}

void PPU::reset() {
}

PPUMode PPU::mode() const {
  return PPUMode{regs.stat.ppu_mode};
}
