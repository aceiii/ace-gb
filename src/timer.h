#pragma once

#include "mmu_device.h"
#include "interrupt_device.h"

struct timer_registers {
  uint16_t div;
  uint8_t tima;
  uint8_t tma;
  union {
    uint8_t tac;
    struct {
      uint8_t clock_select : 2;
      uint8_t enable_tima : 1;
      uint8_t : 5;
    };
  };
};

class Timer : public MmuDevice {
public:
  explicit Timer(InterruptDevice &interrupts);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void execute(uint8_t cycles);

private:
  InterruptDevice &interrupts;
  uint16_t tima_counter;
  timer_registers regs;
};
