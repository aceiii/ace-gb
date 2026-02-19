#pragma once

#include <array>
#include <vector>

#include "memory_bank_controller.h"

class NoMbc : public MemoryBankController {
public:
  explicit NoMbc() = default;
  explicit NoMbc(const std::vector<uint8_t> &bytes);

  [[nodiscard]] uint8_t ReadRom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRam(uint16_t addr) const override;

  void WriteReg(uint16_t addr, uint8_t byte) override;
  void WriteRam(uint16_t addr, uint8_t byte) override;

private:
  std::array<uint8_t, 32 * 1024> rom {};
  std::array<uint8_t, 8 * 1024> ram {};
};
