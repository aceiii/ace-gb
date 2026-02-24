#pragma once

#include <array>

#include "mmu_device.hpp"


class WramDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

public:
  std::array<u8, 8192> wram;
};
