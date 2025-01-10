#pragma once

#include "decoder.h"
#include "registers.h"
#include "memory.h"
#include "immu.h"

#include <memory>
#include <valarray>
#include <vector>

constexpr size_t kClockSpeed = 4194304;

enum class Interrupt {
  VBlank = 0,
  Stat,
  Timer,
  Serial,
  Joypad,
  Count,
};

struct State {
  bool ime = false;
  bool halt = false;
  bool stop = false;
  bool hard_lock = false;
};

class CPU {
public:
  void init();
  void reset();
  uint8_t execute(IMMU *mmu);
  uint8_t read_next8(IMMU *mmu);
  uint16_t read_next16(IMMU *mmu);

public:
  Registers regs {};
  State state {};

private:
  uint8_t execute_interrupts(IMMU *mmu);
};
