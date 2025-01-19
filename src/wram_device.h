#pragma once

#include <array>

#include "mmu_device.h"

class WramDevice : public MmuDevice {
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

public:
  std::array<uint8_t, 8192> wram;
};
