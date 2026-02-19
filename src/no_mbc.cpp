#include "no_mbc.h"
#include <algorithm>

NoMbc::NoMbc(const std::vector<uint8_t> &bytes) {
  std::copy_n(bytes.begin(), std::min(rom_.size(), bytes.size()), rom_.begin());
}

uint8_t NoMbc::ReadRom0(uint16_t addr) const {
  return rom_[addr];
}

uint8_t NoMbc::ReadRom1(uint16_t addr) const {
  return rom_[addr];
}

uint8_t NoMbc::ReadRam(uint16_t addr) const {
  return ram_[addr & 0x1fff];
}

void NoMbc::WriteReg(uint16_t addr, uint8_t byte) {
}

void NoMbc::WriteRam(uint16_t addr, uint8_t byte) {
  ram_[addr & 0x1fff] = byte;
}
