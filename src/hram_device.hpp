#pragma once

#include <array>

#include "types.hpp"
#include "mmu_device.hpp"


constexpr int kHramStart = 0xff80;
constexpr int kHramEnd = 0xfffe;
constexpr int kHramSize = kHramEnd - kHramStart + 1;

class HramDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

private:
  std::array<u8, kHramSize> ram_ {};
};
