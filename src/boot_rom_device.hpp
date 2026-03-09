#pragma once

#include <span>
#include <vector>

#include "types.hpp"
#include "mmu.hpp"


class BootRomDevice : public MmuDevice {
public:
  void LoadBytes(std::span<const u8> bytes);

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void SetDisable(u8 byte);

private:
  std::vector<u8> rom_ {};
  u8 disable_ = 0;
};
