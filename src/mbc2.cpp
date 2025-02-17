#include <utility>
#include <spdlog/spdlog.h>

#include "mbc2.h"

Mbc2::Mbc2(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): info {std::move(info)} {
  size_t size_left = bytes.size();
  auto byte_it = bytes.begin();

  for (auto &bank : rom) {
    size_t size = std::min(bank.size(), size_left);
    std::copy_n(byte_it, size, bank.begin());
    size_left -= size;
    byte_it += static_cast<int>(size);
  }
}

uint8_t Mbc2::read_rom0(uint16_t addr) const {
  return rom[0][addr];
}

uint8_t Mbc2::read_rom1(uint16_t addr) const {
  return rom[rom_bank_number % info.rom_num_banks][addr & 0x3fff];
}

uint8_t Mbc2::read_ram(uint16_t addr) const {
  if (!enable_ram) {
    return 0xff;
  }
  return ram[addr & 0x1ff] | 0b11110000;
}

void Mbc2::write_reg(uint16_t addr, uint8_t byte) {
  if (addr > 0x3fff) {
    return;
  }

  auto bit = (addr >> 8) & 0b1;

  if (bit) {
    rom_bank_number = byte & 0b1111;
    if (rom_bank_number == 0) {
      rom_bank_number = 1;
    }
  } else {
    enable_ram = byte == 0x0a;
  }
}

void Mbc2::write_ram(uint16_t addr, uint8_t byte) {
  spdlog::info("write_ram: [{:04x}] = {:02x}", addr, byte);
  if (!enable_ram) {
    return;
  }
  ram[addr & 0x1ff] = byte;
}

