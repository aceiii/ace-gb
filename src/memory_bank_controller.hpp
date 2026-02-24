#pragma once

#include "types.hpp"


constexpr size_t kRomBank00Start = 0x0000;
constexpr size_t kRomBank00End = 0x3FFF;
constexpr size_t kRomBank01Start = 0x4000;
constexpr size_t kRomBank01End = 0x7FFF;
constexpr size_t kExtRamStart = 0xA000;
constexpr size_t kExtRamEnd = 0xBFFF;

class MemoryBankController {
public:
  virtual ~MemoryBankController() = default;

  [[nodiscard]] virtual u8 ReadRom0(u16 addr) const = 0;
  [[nodiscard]] virtual u8 ReadRom1(u16 addr) const = 0;
  [[nodiscard]] virtual u8 ReadRam(u16 addr) const = 0;

  virtual void WriteReg(u16 addr, u8 byte) = 0;
  virtual void WriteRam(u16 addr, u8 byte) = 0;
};
