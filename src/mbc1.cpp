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

//  spdlog::info("reading rom0: {:04x}", ((addr & 0x3fff) | (ram_bank_number << 19)));
  return ((addr & 0x3fff) | (ram_bank_number << 19));
}

uint8_t Mbc1::read_rom1(uint16_t addr) const {
  uint16_t bank_addr = (addr & 0x3fff) | (rom_bank_number << 14);
  if (info.rom_num_banks > 32) {
    bank_addr |= ram_bank_number << 19;
  }

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
    if ((byte & 0b11111) == 0) {
      rom_bank_number = 1;
      return;
    }

    rom_bank_number = (byte & 0b11111) & ((info.rom_size_bytes / 16384) - 1);
    spdlog::info("write_reg: [{:04x}] = {:02x} -> {}", addr, byte, rom_bank_number);
    return;
  }

  if (addr <= 0x5fff) {
    ram_bank_number = byte & 0b11;
    return;
  }

  banking_mode = byte & 0b1;
}

void Mbc1::write_ram(uint16_t addr, uint8_t byte) {
  if (!ram_enable || !info.ram_size_bytes) {
    return;
  }
  ram[ram_bank_addr(addr)] = byte;
}

uint16_t Mbc1::ram_bank_addr(uint16_t addr) const {
  if (!banking_mode) {
    return addr & 0x1fff;
  }
  return ((addr & 0x1fff) | (ram_bank_number << 13)) & (info.ram_size_bytes-1);


//  if (!banking_mode) {
//    return addr - kExtRamStart;
//  }
//  if (info.ram_num_banks <= 1) {
//    return (addr - kExtRamStart) % info.ram_size_bytes;
//  }
//  return (0x2000 * ram_bank_number) + (addr - kExtRamStart);
}
