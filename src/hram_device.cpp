#include "hram_device.h"

bool HramDevice::valid_for(uint16_t addr) const {
  return addr >= kHramStart && addr <= kHramEnd;
}

void HramDevice::write8(uint16_t addr, uint8_t byte) {
  ram[addr - kHramStart] = byte;
}

uint8_t HramDevice::read8(uint16_t addr) const {
  return ram[addr - kHramStart];
}

void HramDevice::reset() {
  ram.fill(0);
}
