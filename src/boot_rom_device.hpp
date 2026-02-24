#pragma once

#include <array>
#include <vector>

#include "types.hpp"
#include "mmu.hpp"

constexpr size_t kBootRomSize = 256;

using RomBuffer = std::array<u8, kBootRomSize>;

class BootRomDevice : public MmuDevice {
public:
  void LoadBytes(const RomBuffer& bytes);

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void SetDisable(u8 byte);

private:
  RomBuffer rom_ {};
  u8 disable_ = 0;
};
