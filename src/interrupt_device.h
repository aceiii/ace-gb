#pragma once

#include "io.h"
#include "mmu_device.h"
#include "interrupt.h"

struct interrupt_register {
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

  inline void reset() {
    val = 0;
  }
};

class InterruptDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void Reset() override;

  void enable_interrupt(Interrupt interrupt);
  void disable_interrupt(Interrupt interrupt);
  void request_interrupt(Interrupt interrupt);
  void clear_interrupt(Interrupt interrupt);

  [[nodiscard]] bool is_requested(Interrupt interrupt) const;

private:
  interrupt_register flag;
  interrupt_register enable;
};
