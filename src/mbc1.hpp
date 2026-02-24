#pragma once

#include <cstdint>
#include <vector>

#include "types.hpp"
#include "cart_info.hpp"
#include "memory_bank_controller.hpp"


class Mbc1 : public MemoryBankController {
public:
  explicit Mbc1(const std::vector<u8>& bytes, CartInfo info, bool has_ram, bool has_battery);

  [[nodiscard]] u8 ReadRom0(u16 addr) const override;
  [[nodiscard]] u8 ReadRom1(u16 addr) const override;
  [[nodiscard]] u8 ReadRam(u16 addr) const override;

  void WriteReg(u16 addr, u8 byte) override;
  void WriteRam(u16 addr, u8 byte) override;

private:
  using rom_bank = std::array<u8, 16384>;
  using ram_bank = std::array<u8, 8192>;

  std::array<rom_bank, 128> rom_ {};
  std::array<ram_bank, 4> ram_ {};

  CartInfo info_;
  bool mbc1m_ = false;
  bool ram_enable_ = false;

  struct {
    u8 rom_bank_number : 5 = 0;
    u8 ram_bank_number : 2 = 0;
  };
  u8 banking_mode_ = 0;
};
