#pragma once

#include "decoder.h"
#include "registers.h"
#include "memory.h"

#include <memory>
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

public:
  std::unique_ptr<uint8_t[]> memory;
  Registers regs {};
  State state {};
};
