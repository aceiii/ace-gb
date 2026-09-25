#include "command_parser.hpp"

using namespace app;


Command CommandParser::Parse(std::string_view line) {
  if (line.starts_with("load ")) {
    return Command{
      .type = CommandType::Load,
      .path = line.substr(5),
    };
  }

  if (line.starts_with("reset")) {
    return Command{
      .type = CommandType::Reset,
    };
  }

  if (line.starts_with("step")) {
    return Command{
      .type = CommandType::Step,
      .steps = 1,
    };
  }

  if (line.starts_with("read")) {
    return Command{
      .type = CommandType::Read,
      .address = 0xFF,
    };
  };

  if (line.starts_with("write")) {
    return Command{
      .type = CommandType::Write,
      .address = 0xFF,
      .value = 0x12,
    };
  }

  if (line == "quit" || line == "exit") {
    return Command{
      .type = CommandType::Quit,
    };
  }

  if (line == "print") {
    return Command{
      .type = CommandType::Print,
    };
  }

  return Command{
    .type = CommandType::Unknown,
    .line = line,
  };
}
