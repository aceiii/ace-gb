#pragma once

#include <array>

#include "mmu_device.hpp"

class WramDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

public:
  std::array<uint8_t, 8192> wram;
};
