#pragma once

#include <array>
#include <cstdint>
#include <tl/expected.hpp>

#include "mmu_device.h"

class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void load_cartridge(const std::vector<uint8_t> &bytes);

private:
  std::vector<uint8_t> cart_rom;
  std::array<uint8_t, 8192> ext_ram;
};
