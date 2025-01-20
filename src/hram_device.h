#pragma once

#include <array>

#include "mmu_device.h"

constexpr int kHramStart = 0xff80;
constexpr int kHramEnd = 0xfffe;
constexpr int kHramSize = kHramEnd - kHramStart + 1;

class HramDevice : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

private:
  std::array<uint8_t, kHramSize> ram;
};
