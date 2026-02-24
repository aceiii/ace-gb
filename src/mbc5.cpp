#include <utility>
#include <spdlog/spdlog.h>

#include "mbc5.hpp"

constexpr u16 kLogoStart = 0x0104;
constexpr u16 kLogoEnd = 0x0133;

Mbc5::Mbc5(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery, bool has_rumble): info_ {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto& bank : rom_) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

u8 Mbc5::ReadRom0(u16 addr) const {
  return rom_[0][addr];
}

u8 Mbc5::ReadRom1(u16 addr) const {
  return rom_[rom_bank_number_ % info_.rom_num_banks][addr & 0x3fff];
}

u8 Mbc5::ReadRam(u16 addr) const {
  if (!ram_enable_) {
    return 0xff;
  }
  return ram_[ram_bank_number_ % info_.ram_num_banks][addr & 0x1fff];
}

void Mbc5::WriteReg(u16 addr, u8 byte) {
  if (addr <= 0x1fff) {
    ram_enable_ = (byte & 0b1111) == 0x0a;
  } else if (addr <= 0x2fff) {
    rom_bank_number_ = (rom_bank_number_ & 0xff00) | byte;
  } else if (addr <= 0x3fff) {
    rom_bank_number_ = (rom_bank_number_ & 0x00ff) | ((byte & 0b1) << 8);
  } else if (addr <= 0x5fff) {
    ram_bank_number_ = byte;
  }
}

void Mbc5::WriteRam(u16 addr, u8 byte) {
  if (!ram_enable_) {
    return;
  }
  ram_[ram_bank_number_ % info_.ram_num_banks][addr & 0x1fff] = byte;
}
