#include "hram_device.hpp"

bool HramDevice::IsValidFor(u16 addr) const {
  return addr >= kHramStart && addr <= kHramEnd;
}

void HramDevice::Write8(u16 addr, u8 byte) {
  ram_[addr - kHramStart] = byte;
}

u8 HramDevice::Read8(u16 addr) const {
  return ram_[addr - kHramStart];
}

void HramDevice::Reset() {
  ram_.fill(0);
}
