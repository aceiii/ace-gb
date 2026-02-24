#pragma once

#include "types.hpp"


class MmuDevice;
using MmuDevicePtr = MmuDevice*;

class MmuDevice {
public:
  virtual ~MmuDevice() = default;

  [[nodiscard]] virtual bool IsValidFor(u16 addr) const = 0;
  virtual void Write8(u16 addr, u8 byte) = 0;
  [[nodiscard]] virtual u8 Read8(u16 addr) const = 0;
  virtual void Reset() = 0;
};
