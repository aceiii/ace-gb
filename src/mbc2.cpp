#include <utility>
#include <spdlog/spdlog.h>

#include "mbc2.hpp"

Mbc2::Mbc2(const std::vector<uint8_t>& bytes, CartInfo info, bool has_ram, bool has_battery): info_ {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto& bank : rom_) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc2::ReadRom0(uint16_t addr) const {
  return rom_[0][addr];
}

uint8_t Mbc2::ReadRom1(uint16_t addr) const {
  return rom_[rom_bank_number_ % info_.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc2::ReadRam(uint16_t addr) const {
  if (!ram_enable_) {
    return 0xff;
  }
  return ram_[addr & 0x1ff] | 0b11110000;
}

void Mbc2::WriteReg(uint16_t addr, uint8_t byte) {
  if (addr > 0x3fff) {
    return;
  }

  auto bit = (addr >> 8) & 0b1;

  if (bit) {
    rom_bank_number_ = byte & 0b1111;
    if (rom_bank_number_ == 0) {
      rom_bank_number_ = 1;
    }
  } else {
    ram_enable_ = (byte & 0b1111) == 0x0a;
  }
}

void Mbc2::WriteRam(uint16_t addr, uint8_t byte) {
  spdlog::info("write_ram: [{:04x}] = {:02x}", addr, byte);
  if (!ram_enable_ || addr > 0xa1ff) {
    return;
  }
  ram_[addr & 0x1ff] = byte;
}

