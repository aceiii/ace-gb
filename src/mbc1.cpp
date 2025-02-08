#include "mbc1.h"

#include <utility>

Mbc1::Mbc1(std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): rom { bytes }, info {std::move(info)} {
}

uint8_t Mbc1::read8(uint16_t addr) const {
  if (addr <= kRomBank00End) {
    if (!banking_mode) {
      return rom[addr];
    }
  }

  if (addr >= kRomBank01End && addr <= kRomBank01End) {
    auto lower_bank = rom_bank_number;
    if (lower_bank == 0) {
      lower_bank = 1;
    }

    auto rom_bank_addr = (addr & 0x3fff) | (lower_bank << 13) | (ram_bank_number << 18);
    return rom[rom_bank_addr];
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    if (!ram_enable) {
      return 0;
    }

    if (!banking_mode) {
      return ram[addr - kExtRamStart];
    }

    auto ram_bank_addr = ((addr & 0x1fff) | (ram_bank_number << 12)) - kExtRamStart;
    return ram[ram_bank_addr];
  }

  return 0;
}

void Mbc1::write8(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    if (byte == 0xa) {
      ram_enable = true;
    }
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

  if (addr <= 0x7fff) {
    banking_mode = byte & 0x1;
    return;
  }
}
