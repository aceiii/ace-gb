#pragma once

#include <string>

#include "instructions.h"
#include "registers.h"
#include "opcodes.h"

namespace Decoder {
  Instruction decode(uint8_t byte);
  Instruction decode_prefixed(uint8_t byte);
};
