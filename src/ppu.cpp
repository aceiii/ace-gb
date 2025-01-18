#include <cstdint>
#include <utility>
#include <spdlog/spdlog.h>

#include "ppu.h"
#include "cpu.h"

namespace {

constexpr uint16_t kLCDWidth = 160;
constexpr uint16_t kLCDHeight = 144;

constexpr uint16_t kVRAMAddrStart = 0x8000;
constexpr uint16_t kVRAMAddrEnd = 0x9FFF;

constexpr uint16_t kOAMAddrStart = 0xFE00;
constexpr uint16_t kOAMAddrEnd = 0xFE9F;

constexpr std::array<Color, 4> kLCDPalette {
  Color { 240, 240, 240, 255 },
  Color { 178, 178, 178, 255 },
  Color { 100, 100, 100, 255 },
  Color { 48, 48, 48, 255 },
};

}

inline uint16_t addr_mode_8000(uint8_t addr) {
  return 0x8000 + addr;
}

inline uint16_t addr_mode_8800(uint8_t addr) {
  return 0x9000 + static_cast<int8_t>(addr);
}

void Ppu::init() {
  for (int i = 0; i < targets.size(); i += 1) {
    targets[i] = LoadRenderTexture(kLCDWidth, kLCDHeight);
    BeginTextureMode(targets[i]);
    ClearBackground(Color(75, 0, 1, 255));
    EndTextureMode();
  }

  constexpr int tiles_width = 24;
  constexpr int tiles_height = 16;

  target_tiles = LoadRenderTexture(tiles_width * 8, tiles_height * 8);
  BeginTextureMode(target_tiles);
  ClearBackground(BLANK);
  EndTextureMode();
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

const RenderTexture2D* Ppu::target() const {
  return &targets[target_index];
}

const RenderTexture2D* Ppu::bg() const {
  return &target_bg;
}

const RenderTexture2D* Ppu::window() const {
  return &target_window;
}

void Ppu::populate_sprite_buffer() {
  if (!regs.lcdc.lcd_display_enable) {
    return;
  }
}

void Ppu::update_tiles_target() {

}

bool Ppu::valid_for(uint16_t addr) const {
  if (addr >= kVRAMAddrStart && addr <= kVRAMAddrEnd) {
    return true;
  }

  if (addr >= kOAMAddrStart && addr <= kOAMAddrEnd) {
    return true;
  }

  if (addr >= std::to_underlying(IO::LCDC) && addr <= std::to_underlying(IO::LYC)) {
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

  switch (addr) {
    case std::to_underlying(IO::LCDC):
       regs.lcdc.val = byte;
       return;
    case std::to_underlying(IO::STAT):
      regs.stat.val = byte;
      return;
    case std::to_underlying(IO::SCY):
      regs.scy = byte;
      return;
    case std::to_underlying(IO::SCX):
      regs.scx = byte;
      return;
    case std::to_underlying(IO::LY):
      regs.ly = byte;
      return;
    case std::to_underlying(IO::LYC):
      regs.lyc = byte;
      return;
    default: return;
  }
}

[[nodiscard]] uint8_t Ppu::read8(uint16_t addr) const {
  if (addr >= kOAMAddrStart && addr < kOAMAddrEnd) {
    return oam.bytes[addr - kOAMAddrStart];
  }

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

void Ppu::reset() {
  vram.reset();
  oam.reset();
  regs.reset();
  num_sprites = 0;
  target_index = 0;
  cycle_counter = 0;
}

PPUMode Ppu::mode() const {
  return static_cast<PPUMode>(regs.stat.ppu_mode);
}
