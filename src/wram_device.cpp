#include <utility>

#include "wram_device.hpp"
#include "io.hpp"


namespace {
  constexpr size_t kWramStart = 0xC000;
  constexpr size_t kWramBank1 = 0xD000;
  constexpr size_t kWramEnd = 0xDFFF;
  constexpr size_t kEchoRamStart = 0xE000;
  constexpr size_t kEchoRamEnd = 0xFDFF;
  constexpr u16 kWramBankMask = 0x1FFF;
  constexpr u16 kWramIndexMask = 0xFFF;
}

void WramDevice::Init(Mmu* mmu) {
  mmu_ = mmu;
}

bool WramDevice::IsValidFor(u16 addr) const {
  return (addr >= kWramStart && addr <= kEchoRamEnd) || addr == std::to_underlying(IO::SVBK);
}

void WramDevice::Write8(u16 addr, u8 byte) {
  if (addr == std::to_underlying(IO::SVBK)) {
    if (hardware_mode() == HardwareMode::kDmgMode) {
      return;
    }
    svbk_ = byte;
    return;
  }

  BankAt(addr).at(addr & kWramIndexMask) = byte;
}

u8 WramDevice::Read8(u16 addr) const {
  if (addr == std::to_underlying(IO::SVBK)) {
    if (hardware_mode() == HardwareMode::kDmgMode) {
      return 0xFF;
    }
    return svbk_;
  }

  return BankAt(addr).at(addr & kWramIndexMask);
}

void WramDevice::Reset() {
  std::fill_n(banks_.at(0).begin(), banks_.size() * kWramNumBanks, 0);
}

WramBank& WramDevice::Bank0() {
  return banks_.at(0);
}

const WramBank& WramDevice::Bank0() const {
  return banks_.at(0);
}

WramBank& WramDevice::Bank1() {
  u8 bank_idx = svbk_ & 0x7;
  if (bank_idx == 0) {
    bank_idx = 1;
  }

  return banks_.at(bank_idx);
}

const WramBank& WramDevice::Bank1() const {
  u8 bank_idx = svbk_ & 0x7;
  if (bank_idx == 0) {
    bank_idx = 1;
  }

  return banks_.at(bank_idx);
}

WramBank& WramDevice::BankAt(u16 addr) {
  auto offset = addr & kWramBankMask;
  if (offset < kWramBankSize) {
    return Bank0();
  }
  return Bank1();
}

const WramBank& WramDevice::BankAt(u16 addr) const {
  auto offset = addr & kWramBankMask;
  if (offset < kWramBankSize) {
    return Bank0();
  }
  return Bank1();
}
