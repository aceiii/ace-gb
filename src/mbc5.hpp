#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.hpp"
#include "memory_bank_controller.hpp"

class Mbc5 : public MemoryBankController {
public:
  explicit Mbc5(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery, bool has_rumble);

  [[nodiscard]] u8 ReadRom0(u16 addr) const override;
  [[nodiscard]] u8 ReadRom1(u16 addr) const override;
  [[nodiscard]] u8 ReadRam(u16 addr) const override;

  void WriteReg(u16 addr, u8 byte) override;
  void WriteRam(u16 addr, u8 byte) override;

private:
  using rom_bank = std::array<u8, 16384>;
  using ram_bank = std::array<u8, 8192>;

  std::array<rom_bank, 512> rom_ {};
  std::array<ram_bank, 16> ram_ {};

  CartInfo info_;
  bool ram_enable_ = false;

  u16 rom_bank_number_ = 1;
  u8 ram_bank_number_ = 0;
};
