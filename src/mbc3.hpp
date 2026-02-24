#pragma once

#include <cstdint>
#include <vector>

#include "types.hpp"
#include "cart_info.hpp"
#include "memory_bank_controller.hpp"


constexpr size_t kRamBankSize = 8192;

class Mbc3 : public MemoryBankController {
public:
  explicit Mbc3(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery, bool has_timer);

  [[nodiscard]] u8 ReadRom0(u16 addr) const override;
  [[nodiscard]] u8 ReadRom1(u16 addr) const override;
  [[nodiscard]] u8 ReadRam(u16 addr) const override;

  void WriteReg(u16 addr, u8 byte) override;
  void WriteRam(u16 addr, u8 byte) override;

private:
  using rom_bank = std::array<u8, 16384>;
  using ram_bank = std::array<u8, kRamBankSize>;

  std::array<rom_bank, 256> rom_ {};
  std::array<ram_bank, 8> ram_ {};
  CartInfo info_;

  bool ram_enable_ = false;
  u16 rom_bank_number_ = 1;
  std::array<u8, 5> clock_regs_;

  u8* ram_or_clock_ = &ram_[0][0];
  size_t ram_or_clock_mod_ = kRamBankSize;
};
