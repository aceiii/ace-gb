#pragma once

#include "decoder.h"
#include "registers.h"
#include "memory.h"

#include <memory>
#include <valarray>
#include <vector>

struct State {
  bool ime;
  bool halt;
  bool hard_lock;
};

class CPU {
public:
  CPU() = default;

  explicit CPU(size_t mem_size);

  void execute();
  void run();

  uint8_t read8();
  uint16_t read16();

public:
  std::unique_ptr<uint8_t[]> memory;
  Registers regs {};
  State state {};
};
