#pragma once

#include "instructions.h"
#include "registers.h"
#include "opcodes.h"

#include <string>

class Memory;

class Decoder {
public:
  Instruction decode(Memory *memory, uint8_t addr);
};
