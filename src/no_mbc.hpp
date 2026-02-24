#pragma once

#include <array>
#include <vector>

#include "memory_bank_controller.hpp"

class NoMbc : public MemoryBankController {
public:
  explicit NoMbc() = default;
  explicit NoMbc(const std::vector<u8>& bytes);

  [[nodiscard]] u8 ReadRom0(u16 addr) const override;
  [[nodiscard]] u8 ReadRom1(u16 addr) const override;
  [[nodiscard]] u8 ReadRam(u16 addr) const override;

  void WriteReg(u16 addr, u8 byte) override;
  void WriteRam(u16 addr, u8 byte) override;

private:
  std::array<u8, 32 * 1024> rom_ {};
  std::array<u8, 8 * 1024> ram_ {};
};
