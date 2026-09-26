#pragma once

#include <expected>
#include <string>
#include <string_view>

#include "command.hpp"


namespace CommandParser {
  struct ParseError {
    std::string_view line;
    std::string message;
  };

  using ParseResult = std::expected<app::Command, ParseError>;

  ParseResult Parse(std::string_view line);
}
