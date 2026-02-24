#pragma once

#include "mmu_device.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"


struct TimerRegisters {
  u16 div;
  u8 tima;
  u8 tma;
  union {
    u8 tac;
    struct {
      u8 clock_select : 2;
      u8 enable_tima : 1;
      u8 : 5;
    };
  };
};

class Timer : public MmuDevice, public SyncedDevice {
public:
  explicit Timer(InterruptDevice& interrupts);

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void Execute(u8 cycles);
  void OnTick() override;

  u16 div() const;

private:
  InterruptDevice& interrupts_;
  u16 tima_counter_;
  TimerRegisters regs_;
};
