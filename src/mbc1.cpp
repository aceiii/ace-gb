#include <utility>
#include <spdlog/spdlog.h>

#include "mbc1.h"

Mbc1::Mbc1(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery): info {std::move(info)} {
  std::copy_n(bytes.begin(), std::min(rom.size(), bytes.size()), rom.begin());
}

uint8_t Mbc1::read_rom0(uint16_t addr) const {
  if (!banking_mode || info.rom_num_banks <= 32) {
//    spdlog::info("read_dom0: bm:0, addr:{:04x} -> {:04x}", addr, (addr & 0x3fff));
    return rom[addr & 0x3fff];
  }

//  spdlog::info("read_dom0: bm:1, addr:{:04x} -> {:04x}", addr, (addr & 0x3fff) | (ram_bank_number << 19));
  return ((addr & 0x3fff) | (ram_bank_number << 19));
}

uint8_t Mbc1::read_rom1(uint16_t addr) const {
  uint16_t bank_addr = (addr & 0x3fff) | (rom_bank_number << 14);
  if (info.rom_num_banks > 32) {
    bank_addr |= ram_bank_number << 19;
  }

//  spdlog::info("read_dom1: addr:{:04x} -> {:04x}", addr, bank_addr & ((info.rom_num_banks * 16384) - 1));
  return rom[bank_addr];
}

uint8_t Mbc1::read_ram(uint16_t addr) const {
  if (!ram_enable | !info.ram_size_bytes) {
//    spdlog::info("read_ram: enable:0, addr:{:04x}", addr);
    return 0xff;
  }
//  spdlog::info("read_ram: enable:1, bm:{}, addr:{:04x} -> {:04x}", banking_mode, addr, ram_bank_addr(addr));
  return ram[ram_bank_addr(addr)];
}

void Mbc1::write_reg(uint16_t addr, uint8_t byte) {
  if (addr <= 0x1fff) {
    spdlog::info("write_reg: addr:{:04x}, byte:{:02x}, ram_enable:{:02x}", addr, byte, (byte & 0b1111) == 0xa);
    ram_enable = (byte & 0b1111) == 0xa;
    return;
  }

  if (addr <= 0x3fff) {
    if ((byte & 0b11111) == 0) {
      spdlog::info("write_reg: addr:{:04x}, rom_bank_number:{:02x}", addr, 1);
      rom_bank_number = 1;
      return;
    }

    spdlog::info("write_reg: addr:{:04x}, rom_bank_number:{:02x}", addr, byte & 0b11111);
    rom_bank_number = byte & 0b11111 & (info.rom_size_bytes - 1);
    return;
  }

  if (addr <= 0x5fff) {
    spdlog::info("write_reg: addr:{:04x}, ram_bank_number:{:02x}", addr, byte & 0b11);
    ram_bank_number = byte & 0b11;
    return;
  }

  spdlog::info("write_reg: addr:{:04x}, banking_mode:{:02x}", addr, byte & 0b1);
  banking_mode = byte & 0b1;
}

void Mbc1::write_ram(uint16_t addr, uint8_t byte) {
  if (!ram_enable || !info.ram_size_bytes) {
//    spdlog::info("write_ram: enabled:0, addr:{:04x}", addr);
    return;
  }
//  spdlog::info("write_ram: enabled:1, bm:{}, addr:{:04x} -> {:04x}", banking_mode, addr, ram_bank_addr(addr));
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
