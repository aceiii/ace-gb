#pragma once

#include <cstdint>
#include <memory>

#include "cart_info.hpp"
#include "mmu_device.hpp"
#include "memory_bank_controller.hpp"
#include "no_mbc.h"


class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void LoadCartBytes(const std::vector<uint8_t> &bytes);
  const CartInfo& GetCartridgeInfo() const;

private:
  std::unique_ptr<MemoryBankController> mbc_ = std::make_unique<NoMbc>();
  CartInfo info_ {};
};
