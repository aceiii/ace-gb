#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <tl/expected.hpp>

#include "cart_info.h"
#include "mmu_device.h"
#include "memory_bank_controller.h"

class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void load_cartridge(const std::vector<uint8_t> &bytes);
  const cart_info& cartridge_info() const;

private:
  std::vector<uint8_t> cart_rom;
  std::array<uint8_t, 8192> ext_ram;
  cart_info info;
  std::unique_ptr<MemoryBankController> mbc;
};
