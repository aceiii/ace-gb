#include <algorithm>

#include "mmu.h"
#include "memory.h"

MMU::~MMU() = default;

void MMU::write(uint16_t addr, uint8_t byte) {
  if (addr < 0x8000 || (addr >= 0xff4c && addr < 0xff80)) {
    return;
  }

  if (addr >= 0xe000 && addr < 0xfe00) {
    addr = 0xc000 + (addr & 0x1fff);
  }

  if (callbacks.contains(addr)) {
    memory[addr] = callbacks[addr](addr, byte);
  } else {
    memory[addr] = byte;
  }
}

void MMU::write(uint16_t addr, uint16_t word) {
  Mem::set16(this, addr, word);
}

uint8_t MMU::read8(uint16_t addr) const {
  if (addr < 256 && !memory[0xff50]) {
    return boot_rom[addr];
  }

  if (addr >= 0xe000 && addr < 0xfe00) {
    addr = 0xc000 + (addr & 0x1fff);
  }

  return memory[addr];
}

uint16_t MMU::read16(uint16_t addr) const {
  return Mem::get16(this, addr);
}

void MMU::reset() {
  std::fill(begin(memory), end(memory), 0);
}

void MMU::inc(uint16_t addr) {
  memory[addr] += 1;
}

void MMU::on_write8(uint16_t addr, mmu_callback callback) {
  callbacks[addr] = callback;
}

void MMU::load_boot_rom(const uint8_t *rom) {
  std::copy(rom, rom + boot_rom.size(), begin(boot_rom));
}

void MMU::load_cartridge(std::vector<uint8_t> cart) {
  this->cart = cart;
}
