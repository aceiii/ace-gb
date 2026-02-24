#pragma once

#include <cstdint>
#include <vector>

#include "types.hpp"
#include "cart_info.hpp"
#include "memory_bank_controller.hpp"


class Mbc2 : public MemoryBankController {
public:
  explicit Mbc2(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery);

  [[nodiscard]] u8 ReadRom0(u16 addr) const override;
  [[nodiscard]] u8 ReadRom1(u16 addr) const override;
  [[nodiscard]] u8 ReadRam(u16 addr) const override;

  void WriteReg(u16 addr, u8 byte) override;
  void WriteRam(u16 addr, u8 byte) override;

private:
  using rom_bank = std::array<u8, 16384>;

  std::array<rom_bank, 16> rom_ {};
  std::array<u8, 512> ram_ {};
  CartInfo info_;

  bool ram_enable_ = false;
  u16 rom_bank_number_ = 1;
};
