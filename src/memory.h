#pragma once

#include <array>

namespace Mem {
  inline std::unique_ptr<uint8_t[]> create_memory(size_t size) {
    return std::make_unique<uint8_t[]>(size);
  }

  inline void set16(uint8_t* bytes, uint16_t address, uint16_t val) {
    bytes[address] = val >> 8;
    bytes[address + 1] = val & 0xff;
  }

  inline uint16_t get16(const uint8_t* bytes, uint16_t address) {
    return (bytes[address] << 8) | (bytes[address + 1]);
  }
}
