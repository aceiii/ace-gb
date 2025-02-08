#pragma once

#include <cstdint>
#include <vector>

#include "cart_info.h"
#include "memory_bank_controller.h"

class Mbc1 : public MemoryBankController {
public:
  explicit Mbc1(std::vector<uint8_t> &bytes, cart_info info, bool has_ram, bool has_battery);

  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;

private:
  std::vector<uint8_t> &rom;
  cart_info info;
  std::array<uint8_t, 1024 * 4> ram;

  bool ram_enable = false;
  struct {
    uint8_t rom_bank_number: 5 = 0;
    uint8_t ram_bank_number : 2 = 0;
  };
  uint8_t banking_mode = 0;
};
