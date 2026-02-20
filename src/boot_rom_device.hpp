#pragma once

#include <array>
#include <vector>

#include "mmu.hpp"

constexpr size_t kBootRomSize = 256;

using RomBuffer = std::array<uint8_t, kBootRomSize>;

class BootRomDevice : public MmuDevice {
public:
  void LoadBytes(const RomBuffer& bytes);

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void SetDisable(uint8_t byte);

private:
  RomBuffer rom_ {};
  uint8_t disable_ = 0;
};
