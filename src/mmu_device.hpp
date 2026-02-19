#pragma once

#include <cstdint>


class MmuDevice;
using MmuDevicePtr = MmuDevice*;

class MmuDevice {
public:
  virtual ~MmuDevice() = default;

  [[nodiscard]] virtual bool IsValidFor(uint16_t addr) const = 0;
  virtual void Write8(uint16_t addr, uint8_t byte) = 0;
  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  virtual void Reset() = 0;
};
