#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.hpp"
#include "memory_bank_controller.hpp"

class Mbc2 : public MemoryBankController {
public:
  explicit Mbc2(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery);

  [[nodiscard]] uint8_t ReadRom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRam(uint16_t addr) const override;

  void WriteReg(uint16_t addr, uint8_t byte) override;
  void WriteRam(uint16_t addr, uint8_t byte) override;

private:
  using rom_bank = std::array<uint8_t, 16384>;

  std::array<rom_bank, 16> rom {};
  std::array<uint8_t, 512> ram {};
  CartInfo info;

  bool ram_enable = false;
  uint16_t rom_bank_number = 1;
};
