#pragma once

#include "instructions.hpp"
#include "registers.h"


namespace Decoder {
  Instruction Decode(uint8_t byte);
  Instruction DecodePrefixed(uint8_t byte);
};
