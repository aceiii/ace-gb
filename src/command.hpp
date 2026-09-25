#pragma once

#include <string_view>

#include "types.hpp"


namespace app {

enum class CommandType {
  Unknown,
  Load,
  Reset,
  Step,
  Write,
  Read,
  Quit,
};

struct Command {
  CommandType type;
  std::string_view line;
  std::string_view path;
  u16 address;
  u8 value;
  int steps;
};

}
