#pragma once

#include "instructions.hpp"
#include "registers.hpp"


namespace Decoder {
  Instruction Decode(uint8_t byte);
  Instruction DecodePrefixed(uint8_t byte);
};
