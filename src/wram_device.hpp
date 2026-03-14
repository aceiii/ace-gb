#pragma once

#include <array>

#include "mmu_device.hpp"
#include "mmu.hpp"


constexpr size_t kWramBankSize = 4096;
constexpr size_t kWramNumBanks = 8;

using WramBank = std::array<u8, kWramBankSize>;

class WramDevice : public MmuDevice {
public:
  void Init(Mmu* mmu);

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

private:
  WramBank& Bank0();
  const WramBank& Bank0() const;
  WramBank& Bank1();
  const WramBank& Bank1() const;
  WramBank& BankAt(u16 addr);
  const WramBank& BankAt(u16 addr) const;

private:
  Mmu* mmu_ = nullptr;
  std::array<WramBank, kWramNumBanks> banks_;
  u8 svbk_;
};
