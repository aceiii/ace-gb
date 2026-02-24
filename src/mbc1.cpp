#include <utility>
#include <spdlog/spdlog.h>

#include "mbc1.hpp"

constexpr u16 kLogoStart = 0x0104;
constexpr u16 kLogoEnd = 0x0133;

Mbc1::Mbc1(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery): info_ {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto& bank : rom_) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }

  if (info.rom_num_banks >= 16 && std::equal(rom_[0].begin() + kLogoStart, rom_[0].begin() + kLogoEnd + 1, rom_[15].begin() + kLogoStart)) {
    mbc1m_ = true;
    spdlog::info("Multi-Cart detected");
  }
}

u8 Mbc1::ReadRom0(u16 addr) const {
  if (mbc1m_ && banking_mode_) {
      return rom_[(ram_bank_number << 4) % info_.rom_num_banks][addr];
  }

  if (!banking_mode_ || info_.rom_num_banks <= 32) {
    return rom_[0][addr];
  }

  return rom_[(ram_bank_number << 5) % info_.rom_num_banks][addr];
}

u8 Mbc1::ReadRom1(u16 addr) const {
  u16 bank = rom_bank_number & 0b11111;
  if (bank == 0) {
    bank = 1;
  }

  if (mbc1m_) {
    return rom_[((bank & 0b1111) | (ram_bank_number << 4))][addr & 0x3fff];
  }

  if (info_.rom_num_banks > 32) {
    return rom_[(bank | (ram_bank_number << 5)) % info_.rom_num_banks][addr & 0x3fff];
  }

  return rom_[bank % info_.rom_num_banks][addr & 0x3fff];
}

u8 Mbc1::ReadRam(u16 addr) const {
  if (!ram_enable_ | !info_.ram_size_bytes) {
    return 0xff;
  }

  auto bank_idx = !banking_mode_ ? 0 : ram_bank_number % info_.ram_num_banks;
  return ram_[bank_idx][addr & 0x1fff];
}

void Mbc1::WriteReg(u16 addr, u8 byte) {
  if (addr <= 0x1fff) {
    ram_enable_ = (byte & 0b1111) == 0x0a;
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

  banking_mode_ = byte & 0b1;
}

void Mbc1::WriteRam(u16 addr, u8 byte) {
  if (!ram_enable_ || !info_.ram_num_banks) {
    return;
  }

  auto bank_idx = !banking_mode_ ? 0 : ram_bank_number % info_.ram_num_banks;
  ram_[bank_idx][addr & 0x1fff] = byte;
}

