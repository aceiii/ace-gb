#include <charconv>
#include <format>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "command_parser.hpp"
#include "string.hpp"

using namespace CommandParser;

namespace {
  std::tuple<std::string_view, std::string_view> SplitAt(std::string_view line, std::string_view chars) {
    auto idx = line.find_first_of(chars);
    if (idx == std::string_view::npos) {
      return {line, {}};
    }
    return {string::Trim(line.substr(0, idx)), string::Trim(line.substr(idx + 1))};
  }

  std::tuple<std::string_view, std::string_view> SplitFirstWhitespace(std::string_view line) {
    return SplitAt(line, string::kWhiteSpaceChars);
  }

  std::expected<int, std::string> ParseInteger(std::string_view line) {
    int value;
    int offset = 0;
    int base = 10;

    auto prefix = line.substr(0, 2);
    if (prefix == "0x" || prefix == "0X") {
      offset = 2;
      base = 16;
    }

    auto result = std::from_chars(line.data() + offset, line.data() + line.size(), value, base);
    if (result.ec == std::errc{}) {
      return value;
    }

    if (base == 16) {
      return std::unexpected(std::format("Expected an hexadecimal value, but got '{}'", line));
    }

    return std::unexpected(std::format("Expected a decimal value, but got '{}'", line));
  }

  std::expected<int, std::string> ParseAddress(std::string_view line) {
    auto prefix = line.substr(0, 1);
    if (prefix != "@") {
      return std::unexpected(std::format("Expecting address to start with '@' but got '{}'", prefix));
    }
    auto result = ParseInteger(line.substr(1));
    if (!result.has_value()) {
      return result;
    }

    auto value = result.value();
    if (value < 0 || value > 0xFFFF) {
      return std::unexpected("Address must be within the range 0x0000 to 0xFFFF");
    }
    return value;
  }
}

CommandParser::ParseResult CommandParser::Parse(std::string_view line) {
  line = string::Trim(line);
  auto [cmd, args] = SplitFirstWhitespace(line);

  if (cmd == "l" || cmd == "load") {
    return app::Command{
      .type = app::CommandType::Load,
      .path = args,
    };
  }

  if (cmd == "r" || cmd == "reset") {
    return app::Command{
      .type = app::CommandType::Reset,
    };
  }

  if (cmd == "s" || cmd == "step") {
    if (args == "") {
      return app::Command{
        .type = app::CommandType::Step,
        .steps = 1,
      };
    }

    auto step_result = ParseInteger(args);
    if (!step_result.has_value()) {
      return std::unexpected(ParseError{
        .line = line,
        .message = step_result.error(),
      });
    }

    auto steps = step_result.value();
    if (steps < 1) {
      return std::unexpected(ParseError{
        .line = line,
        .message = "Step must be greater than 0",
      });
    }

    return app::Command{
      .type = app::CommandType::Step,
      .steps = steps,
    };
  }

  if (cmd == "r" || cmd == "read") {
    auto addr_result = ParseAddress(args);
    if (!addr_result.has_value()) {
      return std::unexpected(ParseError{
        .line = line,
        .message = addr_result.error(),
      });
    }

    return app::Command{
      .type = app::CommandType::Read,
      .address = static_cast<u16>(addr_result.value()),
    };
  };

  if (cmd == "w" || cmd == "write") {
    auto [addr_part, value_part] = SplitAt(args, "=");

    auto addr_result = ParseAddress(addr_part);
    if (!addr_result.has_value()) {
      return std::unexpected(ParseError{
        .line = line,
        .message = addr_result.error(),
      });
    }

    auto value_result = ParseInteger(value_part);
    if (!value_result.has_value()) {
      return std::unexpected(ParseError{
        .line = line,
        .message = value_result.error(),
      });
    }

    auto value = value_result.value();
    if (value < 0 || value > 0xFF) {
      return std::unexpected(ParseError{
        .line = line,
        .message = "Value must be within range 0x00 to 0xFF",
      });
    }

    return app::Command{
      .type = app::CommandType::Write,
      .address = static_cast<u16>(addr_result.value()),
      .value = static_cast<u8>(value),
    };
  }

  if (cmd == "x" || cmd == "q" || cmd == "quit" || cmd == "exit") {
    return app::Command{
      .type = app::CommandType::Quit,
    };
  }

  if (cmd == "print") {
    return app::Command{
      .type = app::CommandType::Print,
    };
  }

  return std::unexpected(ParseError{
    .line = line,
    .message = std::format("Unknown command '{}'", cmd),
  });
}
