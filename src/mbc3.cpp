#include <utility>
#include <spdlog/spdlog.h>

#include "mbc3.hpp"

Mbc3::Mbc3(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery, bool has_timer): info_ {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto &bank : rom_) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc3::ReadRom0(uint16_t addr) const {
  return rom_[0][addr];
}

uint8_t Mbc3::ReadRom1(uint16_t addr) const {
  return rom_[rom_bank_number_ % info_.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc3::ReadRam(uint16_t addr) const {
  return ram_or_clock_[(addr - 0xa000) % ram_or_clock_mod_];
}

void Mbc3::WriteReg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    ram_enable_ = (byte & 0b1111) == 0x0a;
  } else if (addr <= 0x3fff) {
    rom_bank_number_ = byte;
    if (rom_bank_number_ == 0) {
      rom_bank_number_ = 1;
    }

  } else if (addr <= 0x5fff) {
    if (byte < 0x08) {
      auto &bank = ram_[byte];
      ram_or_clock_ = bank.begin();
      ram_or_clock_mod_ = bank.size();
    } else if (byte >= 0x08 && byte <= 0x0c) {
      ram_or_clock_ = &clock_regs_[byte - 0x08];
      ram_or_clock_mod_ = 1;
    }
  } else if (addr <= 0x7fff) {

  }
}

void Mbc3::WriteRam(uint16_t addr, uint8_t byte) {
  ram_or_clock_[(addr - 0xa000) % ram_or_clock_mod_] = byte;
}

