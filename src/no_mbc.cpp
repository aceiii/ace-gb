#include "no_mbc.hpp"
#include <algorithm>

NoMbc::NoMbc(const std::vector<u8>& bytes) {
  std::copy_n(bytes.begin(), std::min(rom_.size(), bytes.size()), rom_.begin());
}

u8 NoMbc::ReadRom0(u16 addr) const {
  return rom_[addr];
}

u8 NoMbc::ReadRom1(u16 addr) const {
  return rom_[addr];
}

u8 NoMbc::ReadRam(u16 addr) const {
  return ram_[addr & 0x1fff];
}

void NoMbc::WriteReg(u16 addr, u8 byte) {
}

void NoMbc::WriteRam(u16 addr, u8 byte) {
  ram_[addr & 0x1fff] = byte;
}
