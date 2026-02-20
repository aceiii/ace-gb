#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.hpp"
#include "memory_bank_controller.hpp"

class Mbc5 : public MemoryBankController {
public:
  explicit Mbc5(const std::vector<uint8_t>& bytes, CartInfo info, bool has_ram, bool has_battery, bool has_rumble);

  [[nodiscard]] uint8_t ReadRom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t ReadRam(uint16_t addr) const override;

  void WriteReg(uint16_t addr, uint8_t byte) override;
  void WriteRam(uint16_t addr, uint8_t byte) override;

private:
  using rom_bank = std::array<uint8_t, 16384>;
  using ram_bank = std::array<uint8_t, 8192>;

  std::array<rom_bank, 512> rom_ {};
  std::array<ram_bank, 16> ram_ {};

  CartInfo info_;
  bool ram_enable_ = false;

  uint16_t rom_bank_number_ = 1;
  uint8_t ram_bank_number_ = 0;
};
