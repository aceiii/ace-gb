#pragma once

#include <array>
#include <memory>

#include "immu.h"

namespace Mem {
  inline void set16(uint8_t *bytes, uint16_t address, uint16_t val) {
    bytes[address] = val & 0xff;
    bytes[address + 1] = val >> 8;
  }

  inline uint16_t get16(const uint8_t *bytes, uint16_t address) {
    return bytes[address] | (bytes[address + 1] << 8);
  }

  inline void set16(IMMU *mmu, uint16_t addr, uint16_t val) {
    uint8_t lo = val & 0xff;
    uint8_t hi = val >> 8;
    mmu->write(addr, lo);
    mmu->write(addr + 1, hi);
  }

  inline uint16_t get16(const IMMU *mmu, uint16_t addr) {
    auto lo = mmu->read8(addr);
    auto hi = mmu->read8(addr + 1);
    return lo | (hi << 8);
  }

  inline void push(IMMU *mmu, uint16_t &sp, uint8_t val) {
    sp -= 1;
    mmu->write(sp, val);
  }

  inline void push(IMMU *mmu, uint16_t &sp, uint16_t val) {
    sp -= 2;
    mmu->write(sp, val);
  }

  inline uint8_t pop8(const IMMU *mmu, uint16_t &sp) {
    auto result = mmu->read8(sp);
    sp += 1;
    return result;
  }

  inline uint16_t pop16(const IMMU *mmu, uint16_t &sp) {
    auto result = mmu->read16(sp);
    sp += 2;
    return result;
  }
}
