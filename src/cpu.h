#pragma once

#include "decoder.h"
#include "registers.h"
#include "memory.h"
#include "mmu.h"
#include "io.h"
#include "interrupt.h"
#include "interrupt_device.h"

#include <memory>
#include <valarray>
#include <vector>

constexpr size_t kClockSpeed = 4194304;

struct State {
  bool ime = false;
  bool halt = false;
  bool stop = false;
  bool hard_lock = false;

  inline void reset() {
    ime = false;
    halt = false;
    stop = false;
    hard_lock = false;
  }
};

class Cpu {
public:
  Cpu() = delete;
  explicit Cpu(Mmu &mmu, InterruptDevice &interrupts);

  void reset();
  uint8_t execute();
  uint8_t read_next8();
  uint16_t read_next16();

public:
  Mmu &mmu;
  InterruptDevice &interrupts;
  Registers regs;
  State state;

private:
  uint8_t execute_interrupts();
};
