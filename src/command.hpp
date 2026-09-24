#pragma once

namespace app {

enum class CommandType {
  Unknown,
  Load,
  Reset,
  Step,
  Write,
  Read,
};

struct Command {
  CommandType type;
  std::string line;
  std::string message;
};

}
