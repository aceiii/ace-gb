#pragma once

#include "instructions.h"
#include "registers.h"
#include "opcodes.h"

#include <string>

namespace Decoder {
  Instruction decode(uint8_t byte);
  Instruction decode_prefixed(uint8_t byte);
};
