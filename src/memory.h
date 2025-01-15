#pragma once

#include <array>
#include <memory>

namespace Mem {
  inline void set16(uint8_t *bytes, uint16_t address, uint16_t val) {
    bytes[address] = val & 0xff;
    bytes[address + 1] = val >> 8;
  }

  inline uint16_t get16(const uint8_t *bytes, uint16_t address) {
    return bytes[address] | (bytes[address + 1] << 8);
  }
}
