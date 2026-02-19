#pragma once

#include "instructions.h"
#include "registers.h"


namespace Decoder {
  Instruction Decode(uint8_t byte);
  Instruction DecodePrefixed(uint8_t byte);
};
