#pragma once

#include <string_view>

#include "types.hpp"


namespace app {

enum class CommandType {
  Quit,
  Load,
  Reset,
  Step,
  Write,
  Read,
  Print,
  Run,
  BreakpointList,
  BreakpointAdd,
  BreakpointRemove,
};

struct Command {
  enum struct Reg {
    A,
    F,
    B,
    C,
    D,
    E,
    H,
    L,
    AF,
    BC,
    DE,
    HL,
  };

  CommandType type;

  std::string_view path;
  bool is_register;
  Reg reg;
  u16 address;
  int value;
};

}
