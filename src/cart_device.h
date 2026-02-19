#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <memory>

#include "cart_info.h"
#include "mmu_device.hpp"
#include "memory_bank_controller.h"
#include "no_mbc.h"

class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void load_cartridge(const std::vector<uint8_t> &bytes);
  const cart_info& cartridge_info() const;

private:
  std::unique_ptr<MemoryBankController> mbc = std::make_unique<NoMbc>();
  cart_info info;
};
