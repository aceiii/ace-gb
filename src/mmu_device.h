#pragma once

#include <cstdint>

class MmuDevice;
using mmu_device_ptr = MmuDevice*;

class MmuDevice {
public:
  [[nodiscard]] virtual bool valid_for(uint16_t addr) const = 0;
  virtual void write8(uint16_t addr, uint8_t byte) = 0;
  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  virtual void reset() = 0;

  inline void write16(uint16_t address, uint16_t val) {
    write8(address, val & 0xff);
    write8(address + 1, val >> 8);
  }

  [[nodiscard]] inline uint16_t read16(uint16_t address) const {
    return read8(address) | (read8(address + 1) << 8);
  }
};
