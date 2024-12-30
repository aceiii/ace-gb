#pragma once

#include "instructions.h"
#include "registers.h"
#include "opcodes.h"

#include <string>

namespace Decoder {
  Instruction decode(const uint8_t* mem, uint8_t addr);
};
