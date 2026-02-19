#pragma once

#include <array>

#include "mmu_device.hpp"

constexpr int kHramStart = 0xff80;
constexpr int kHramEnd = 0xfffe;
constexpr int kHramSize = kHramEnd - kHramStart + 1;

class HramDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

private:
  std::array<uint8_t, kHramSize> ram;
};
