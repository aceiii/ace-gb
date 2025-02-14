#include <utility>
#include <spdlog/spdlog.h>

#include "mbc1.h"

Mbc1::Mbc1(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): info {std::move(info)} {
  std::copy_n(bytes.begin(), std::min(rom.size(), bytes.size()), rom.begin());
}

uint8_t Mbc1::read_rom0(uint16_t addr) const {
  if (!banking_mode || info.rom_num_banks <= 32) {
    return rom[addr];
  }
  return rom[(addr & 0x3fff) | (ram_bank_number << 19)];
}

uint8_t Mbc1::read_rom1(uint16_t addr) const {
  uint8_t bank = rom_bank_number & 0b11111;
  if (bank == 0) {
    bank = 1;
  }

  bank &= info.rom_num_banks - 1;
  uint16_t bank_addr = (addr & 0x3fff) | (bank << 14) | (ram_bank_number << 19);
  return rom[bank_addr];
}

uint8_t Mbc1::read_ram(uint16_t addr) const {
  if (!ram_enable | !info.ram_size_bytes) {
    return 0xff;
  }
  return ram[ram_bank_addr(addr)];
}

void Mbc1::write_reg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    ram_enable = (byte & 0b1111) == 0xa;
    return;
  }

  if (addr <= 0x3fff) {
    rom_bank_number = byte;
    return;
  }

  if (addr <= 0x5fff) {
    ram_bank_number = byte;
    return;
  }

  banking_mode = byte & 0b1;
}

void Mbc1::write_ram(uint16_t addr, uint8_t byte) {
  if (!ram_enable || !info.ram_num_banks) {
    return;
  }
  ram[ram_bank_addr(addr)] = byte;
}

uint16_t Mbc1::ram_bank_addr(uint16_t addr) const {
  if (!banking_mode) {
    return addr & 0x1fff;
  }
  return (addr & 0x1fff) | (ram_bank_number << 13) & ((info.ram_num_banks << 13) - 1);
}
