#include "command_parser.hpp"

using namespace app;


Command CommandParser::Parse(std::string_view line) {
  return Command{
    .type = CommandType::Unknown,
    .line = std::string(line),
  };
}
