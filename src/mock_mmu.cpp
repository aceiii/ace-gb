#include <algorithm>

#include "mock_mmu.h"
#include "memory.h"

MockMMU::~MockMMU() = default;

void MockMMU::write(uint16_t addr, uint8_t byte) {
  memory[addr] = byte;
}

void MockMMU::write(uint16_t addr, uint16_t word) {
  Mem::set16(memory.data(), addr, word);
}

uint8_t MockMMU::read8(uint16_t addr) const {
  return memory[addr];
}

uint16_t MockMMU::read16(uint16_t addr) const {
  return Mem::get16(memory.data(), addr);
}

void MockMMU::reset() {
  std::fill(begin(memory), end(memory), 0);
}

void MockMMU::inc(uint16_t addr) {
  memory[addr] += 1;
}

void MockMMU::on_write8(uint16_t addr, mmu_callback callback) {
}

void MockMMU::load_boot_rom(const uint8_t *rom) {

}

void MockMMU::load_cartridge(std::vector<uint8_t> cart) {

}
