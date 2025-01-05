#include "mmu.h"
#include "memory.h"

MMU::~MMU() = default;

void MMU::write(uint16_t addr, uint8_t byte) {
  memory[addr] = byte;

  if (callbacks.contains(addr)) {
    callbacks[addr](addr, byte);
  }
}

void MMU::write(uint16_t addr, uint16_t word) {
  Mem::set16(memory.data(), addr, word);
}

uint8_t MMU::read8(uint16_t addr) const {
  return memory[addr];
}

uint16_t MMU::read16(uint16_t addr) const {
  return Mem::get16(memory.data(), addr);
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
