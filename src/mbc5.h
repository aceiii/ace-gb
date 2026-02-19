#pragma once

#include <cstdint>
#include <vector>

#include "CartInfo.h"
#include "memory_bank_controller.h"

class Mbc5 : public MemoryBankController {
public:
  explicit Mbc5(const std::vector<uint8_t> &bytes, CartInfo info, bool has_ram, bool has_battery, bool has_rumble);

  [[nodiscard]] uint8_t read_rom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t read_rom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t read_ram(uint16_t addr) const override;

  void write_reg(uint16_t addr, uint8_t byte) override;
  void write_ram(uint16_t addr, uint8_t byte) override;

private:
  using rom_bank = std::array<uint8_t, 16384>;
  using ram_bank = std::array<uint8_t, 8192>;

  std::array<rom_bank, 512> rom {};
  std::array<ram_bank, 16> ram {};

  CartInfo info;
  bool ram_enable = false;

  uint16_t rom_bank_number = 1;
  uint8_t ram_bank_number = 0;
};
