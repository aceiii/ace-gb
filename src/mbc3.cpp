#include <utility>
#include <spdlog/spdlog.h>

#include "mbc3.h"

Mbc3::Mbc3(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery, bool has_timer): info {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto &bank : rom) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc3::ReadRom0(uint16_t addr) const {
  return rom[0][addr];
}

uint8_t Mbc3::ReadRom1(uint16_t addr) const {
  return rom[rom_bank_number % info.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc3::ReadRam(uint16_t addr) const {
  return ram_or_clock[(addr - 0xa000) % ram_or_clock_mod];
}

void Mbc3::WriteReg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    ram_enable = (byte & 0b1111) == 0x0a;
  } else if (addr <= 0x3fff) {
    rom_bank_number = byte;
    if (rom_bank_number == 0) {
      rom_bank_number = 1;
    }

  } else if (addr <= 0x5fff) {
    if (byte < 0x08) {
      auto &bank = ram[byte];
      ram_or_clock = bank.begin();
      ram_or_clock_mod = bank.size();
    } else if (byte >= 0x08 && byte <= 0x0c) {
      ram_or_clock = &clock_regs[byte - 0x08];
      ram_or_clock_mod = 1;
    }
  } else if (addr <= 0x7fff) {

  }
}

void Mbc3::WriteRam(uint16_t addr, uint8_t byte) {
  ram_or_clock[(addr - 0xa000) % ram_or_clock_mod] = byte;
}

