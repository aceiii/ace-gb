#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.hpp"
#include "memory_bank_controller.h"

constexpr size_t kRamBankSize = 8192;

class Mbc3 : public MemoryBankController {
public:
  explicit Mbc3(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery, bool has_timer);

  [[nodiscard]] uint8_t ReadRom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRam(uint16_t addr) const override;

  void WriteReg(uint16_t addr, uint8_t byte) override;
  void WriteRam(uint16_t addr, uint8_t byte) override;

private:
  using rom_bank = std::array<uint8_t, 16384>;
  using ram_bank = std::array<uint8_t, kRamBankSize>;

  std::array<rom_bank, 256> rom {};
  std::array<ram_bank, 8> ram {};
  CartInfo info;

  bool ram_enable = false;
  uint16_t rom_bank_number = 1;
  std::array<uint8_t, 5> clock_regs;

  uint8_t *ram_or_clock = &ram[0][0];
  size_t ram_or_clock_mod = kRamBankSize;
};
