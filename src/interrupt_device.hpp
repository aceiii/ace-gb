#pragma once

#include "mmu_device.hpp"
#include "interrupt.hpp"

struct InterruptRegister {
  union {
    struct {
      uint8_t vblank: 1;
      uint8_t lcd: 1;
      uint8_t timer: 1;
      uint8_t serial: 1;
      uint8_t joypad: 1;
    };
    uint8_t val;
  };

  void reset() {
    val = 0;
  }
};

class InterruptDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void EnableInterrupt(Interrupt interrupt);
  void DisableInterrupt(Interrupt interrupt);
  void RequestInterrupt(Interrupt interrupt);
  void ClearInterrupt(Interrupt interrupt);

  [[nodiscard]] bool IsInterruptRequested(Interrupt interrupt) const;

private:
  InterruptRegister flag_ {};
  InterruptRegister enable_ {};
};
