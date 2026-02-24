#pragma once

#include "types.hpp"
#include "mmu_device.hpp"
#include "interrupt.hpp"


struct InterruptRegister {
  union {
    struct {
      u8 vblank: 1;
      u8 lcd: 1;
      u8 timer: 1;
      u8 serial: 1;
      u8 joypad: 1;
    };
    u8 val;
  };

  void reset() {
    val = 0;
  }
};

class InterruptDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
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
