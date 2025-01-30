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
};
