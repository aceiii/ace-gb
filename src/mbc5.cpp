#include <utility>
#include <spdlog/spdlog.h>

#include "mbc5.h"

constexpr uint16_t kLogoStart = 0x0104;
constexpr uint16_t kLogoEnd = 0x0133;

Mbc5::Mbc5(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery, bool has_rumble): info {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto &bank : rom) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc5::read_rom0(uint16_t addr) const {
  return rom[0][addr];
}

uint8_t Mbc5::read_rom1(uint16_t addr) const {
  return rom[rom_bank_number % info.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc5::read_ram(uint16_t addr) const {
  if (!ram_enable) {
    return 0xff;
  }
  return ram[ram_bank_number % info.ram_num_banks][addr & 0x1fff];
}

void Mbc5::write_reg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    ram_enable = (byte & 0b1111) == 0x0a;
  } else if (addr <= 0x2fff) {
    rom_bank_number = (rom_bank_number & 0xff00) | byte;
  } else if (addr <= 0x3fff) {
    rom_bank_number = (rom_bank_number & 0x00ff) | ((byte & 0b1) << 8);
  } else if (addr <= 0x5fff) {
    ram_bank_number = byte;
  }
}

void Mbc5::write_ram(uint16_t addr, uint8_t byte) {
  if (!ram_enable) {
    return;
  }
  ram[ram_bank_number % info.ram_num_banks][addr & 0x1fff] = byte;
}
