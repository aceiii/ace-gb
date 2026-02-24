#pragma once

#include <cstdint>
#include <memory>

#include "types.hpp"
#include "cart_info.hpp"
#include "mmu_device.hpp"
#include "memory_bank_controller.hpp"
#include "no_mbc.hpp"


class CartDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void LoadCartBytes(const std::vector<u8>& bytes);
  const CartInfo& GetCartridgeInfo() const;

private:
  std::unique_ptr<MemoryBankController> mbc_ = std::make_unique<NoMbc>();
  CartInfo info_ {};
};
