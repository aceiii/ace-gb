#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>

#include "cart_info.h"
#include "mmu_device.h"
#include "memory_bank_controller.h"
#include "no_mbc.h"

class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void load_cartridge(const std::vector<uint8_t> &bytes);
  const cart_info& cartridge_info() const;

private:
  std::unique_ptr<MemoryBankController> mbc = std::make_unique<NoMbc>();
  cart_info info;
};
