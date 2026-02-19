#include "hram_device.h"

bool HramDevice::IsValidFor(uint16_t addr) const {
  return addr >= kHramStart && addr <= kHramEnd;
}

void HramDevice::Write8(uint16_t addr, uint8_t byte) {
  ram[addr - kHramStart] = byte;
}

uint8_t HramDevice::Read8(uint16_t addr) const {
  return ram[addr - kHramStart];
}

void HramDevice::Reset() {
  ram.fill(0);
}
