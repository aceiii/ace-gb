#include "no_mbc.h"

NoMbc::NoMbc(const std::vector<uint8_t> &bytes) {
  std::copy_n(bytes.begin(), std::min(rom.size(), bytes.size()), rom.begin());
}

uint8_t NoMbc::read_rom0(uint16_t addr) const {
  return rom[addr];
}

uint8_t NoMbc::read_rom1(uint16_t addr) const {
  return rom[addr];
}

uint8_t NoMbc::read_ram(uint16_t addr) const {
  return ram[addr & 0x1fff];
}

void NoMbc::write_reg(uint16_t addr, uint8_t byte) {
}

void NoMbc::write_ram(uint16_t addr, uint8_t byte) {
  ram[addr & 0x1fff] = byte;
}
