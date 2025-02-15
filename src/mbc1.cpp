#include <utility>
#include <spdlog/spdlog.h>

#include "mbc1.h"

Mbc1::Mbc1(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): info {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto &bank : rom) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc1::read_rom0(uint16_t addr) const {
  if (!banking_mode || info.rom_num_banks <= 32) {
    return rom[0][addr];
  }
  return rom[(ram_bank_number << 5) % info.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc1::read_rom1(uint16_t addr) const {
  uint16_t bank = rom_bank_number & 0b11111;
  if (bank == 0) {
    bank = 1;
  }

  if (info.rom_num_banks > 32) {
    return rom[((ram_bank_number << 5) + bank) % info.rom_num_banks][addr & 0x3fff];
  }

  return rom[bank % info.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc1::read_ram(uint16_t addr) const {
  if (!ram_enable | !info.ram_size_bytes) {
    return 0xff;
  }

  auto bank_idx = !banking_mode ? 0 : ram_bank_number % info.ram_num_banks;
  return ram[bank_idx][addr & 0x1fff];
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

  auto bank_idx = !banking_mode ? 0 : ram_bank_number % info.ram_num_banks;
  ram[bank_idx][addr & 0x1fff] = byte;
}

