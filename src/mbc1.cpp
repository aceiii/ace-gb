#include <utility>
#include <spdlog/spdlog.h>

#include "mbc1.h"

Mbc1::Mbc1(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): info {std::move(info)} {
  std::copy_n(bytes.begin(), std::min(rom.size(), bytes.size()), rom.begin());
}

uint8_t Mbc1::read_rom0(uint16_t addr) const {
  if (!banking_mode) {
    return rom[addr & 0x3fff];
  }

  return (addr & 0x3fff) | (ram_bank_number << 19);
}

uint8_t Mbc1::read_rom1(uint16_t addr) const {
  uint16_t bank_addr = (addr & 0x3fff) | rom_bank_number << 14;
  if (info.rom_num_banks > 32) {
    bank_addr |= bank_addr << 19;
  }
  if (bank_addr >= rom.size()) {
    spdlog::warn("Bank addr >= rom.size(): {} > {}", bank_addr, bank_addr);
  }
  return rom[bank_addr];
}

uint8_t Mbc1::read_ram(uint16_t addr) const {
  if (!ram_enable) {
    return 0xff;
  }
  return ram[ram_bank_addr(addr)];
}

void Mbc1::write_reg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    ram_enable = byte == 0xa;
    return;
  }

  if (addr <= 0x3fff) {
    if ((byte & 0x1f) == 0) {
      rom_bank_number = 1;
      return;
    }

    uint8_t mask = 0b1111;
    if (info.rom_num_banks <= 2) {
      mask = 0b1;
    } else if (info.rom_num_banks <= 4) {
      mask = 0b11;
    } else if (info.rom_num_banks <= 8) {
      mask = 0b111;
    } else if (info.rom_num_banks <= 16) {
      mask = 0b1111;
    }

    rom_bank_number = byte & mask;
    return;
  }

  if (addr <= 0x5fff) {
    ram_bank_number = byte & 0b11;
    return;
  }

  banking_mode = byte & 0b1;
}

void Mbc1::write_ram(uint16_t addr, uint8_t byte) {
  if (!ram_enable) {
    return;
  }
  ram[ram_bank_addr(addr)] = byte;
}

uint16_t Mbc1::ram_bank_addr(uint16_t addr) const {
  spdlog::info("reading bank addr: {}", addr);
  if (!banking_mode) {
    return addr & 0x1fff;
  }
  spdlog::info("ram_bank_number: {}", ram_bank_number);
  return((addr & 0x1fff) | (ram_bank_number << 13));
}
