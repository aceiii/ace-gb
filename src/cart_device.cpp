#include "cart_device.h"

namespace {

constexpr size_t kRomBank00Start = 0x0000;
constexpr size_t kRomBank00End = 0x3FFF;
constexpr size_t kRomBank01Start = 0x4000;
constexpr size_t kRomBank01End = 0x7FFF;
constexpr size_t kExtRamStart = 0xA000;
constexpr size_t kExtRamEnd = 0xBFFF;

}

bool CartDevice::valid_for(uint16_t addr) const {
  return addr <= kRomBank01End || (addr >= kExtRamStart && addr <= kExtRamEnd);
}

void CartDevice::write8(uint16_t addr, uint8_t byte) {
  if (addr >= kExtRamEnd && addr <= kExtRamEnd) {
    ext_ram[addr - kExtRamStart] = byte;
  }
}

uint8_t CartDevice::read8(uint16_t addr) const {
  if (addr <= kRomBank01End && addr < cart_rom.size()) {
    return cart_rom[addr];
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    return ext_ram[addr - kExtRamStart];
  }

  return 0;
}

void CartDevice::reset() {
  cart_rom.clear();
  ext_ram.fill(0);
}

void CartDevice::load_cartridge(const std::vector<uint8_t> &bytes) {
  cart_rom = bytes;
}
