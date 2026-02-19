#pragma once

#include "mmu_device.hpp"
#include "interrupt_device.h"
#include "synced_device.hpp"

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

class Timer : public MmuDevice, public SyncedDevice {
public:
  explicit Timer(InterruptDevice &interrupts);

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void Reset() override;

  void execute(uint8_t cycles);
  void OnTick() override;

  uint16_t div() const;

private:
  InterruptDevice &interrupts;
  uint16_t tima_counter;
  timer_registers regs;
};
