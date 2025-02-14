#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.h"
#include "memory_bank_controller.h"

class Mbc1 : public MemoryBankController {
public:
  explicit Mbc1(const std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery);

  [[nodiscard]] uint8_t read_rom0(uint16_t addr) const override;
  [[nodiscard]] uint8_t read_rom1(uint16_t addr) const override;
  [[nodiscard]] uint8_t read_ram(uint16_t addr) const override;

  void write_reg(uint16_t addr, uint8_t byte) override;
  void write_ram(uint16_t addr, uint8_t byte) override;

private:
  [[nodiscard]] uint16_t ram_bank_addr(uint16_t addr) const;

private:
  std::array<uint8_t, 2097152> rom {};
  std::array<uint8_t, 32768> ram {};
  cart_info info;
  bool mbc1m = false;
  bool ram_enable = false;

  struct {
    uint8_t rom_bank_number : 5 = 0;
    uint8_t ram_bank_number : 2 = 0;
  };
  uint8_t banking_mode = 0;
};
