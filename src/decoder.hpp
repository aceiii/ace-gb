#pragma once

#include "types.hpp"
#include "instructions.hpp"
#include "registers.hpp"


namespace Decoder {
  Instruction Decode(u8 byte);
  Instruction DecodePrefixed(u8 byte);
};
