#include <charconv>
#include <format>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <magic_enum/magic_enum.hpp>

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

  std::expected<app::Command::Reg, std::string> ParseRegister(std::string_view line) {
    auto prefix = line.substr(0, 1);
    if (prefix != "$") {
      return std::unexpected(std::format("Expecting register to start with '$' but got '{}", prefix));
    }

    auto result = magic_enum::enum_cast<app::Command::Reg>(line.substr(1), magic_enum::case_insensitive);
    if (result.has_value()) {
      return result.value();
    }
    return std::unexpected(std::format("Expected register but got '{}'", line));
  }

  struct RegisterOrAddress {
    bool is_register;
    app::Command::Reg reg;
    u16 address;
  };

  std::expected<RegisterOrAddress, std::string> ParseRegisterOrAddress(std::string_view line) {
    auto prefix = line.substr(0, 1);
    if (prefix != "@" && prefix != "$") {
      return std::unexpected(std::format("Expecting register or address but got '{}'", line));
    }

    auto register_result = ParseRegister(line);
    if (register_result.has_value()) {
      return RegisterOrAddress{
        .is_register = true,
        .reg = register_result.value(),
      };
    }

    auto address_result = ParseAddress(line);
    if (!address_result.has_value()) {
      return std::unexpected(address_result.error());
    }

    return RegisterOrAddress{
      .is_register = false,
      .address = static_cast<u16>(address_result.value()),
    };
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

  if (cmd == "reset") {
    return app::Command{
      .type = app::CommandType::Reset,
    };
  }

  if (cmd == "s" || cmd == "step") {
    if (args == "") {
      return app::Command{
        .type = app::CommandType::Step,
        .value = 1,
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
      .value = steps,
    };
  }

  if (cmd == "r" || cmd == "read") {
    auto result = ParseRegisterOrAddress(args);
    if (!result.has_value()) {
      return std::unexpected(ParseError{
        .line = line,
        .message = result.error(),
      });
    }

    const auto& reg_or_addr = result.value();

    return app::Command{
      .type = app::CommandType::Read,
      .is_register = reg_or_addr.is_register,
      .reg = reg_or_addr.reg,
      .address = reg_or_addr.address,
    };
  };

  if (cmd == "w" || cmd == "write") {
    auto [addr_part, value_part] = SplitAt(args, "=");

    auto addr_result = ParseRegisterOrAddress(addr_part);
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
    const auto& reg_or_addr = addr_result.value();

    return app::Command{
      .type = app::CommandType::Write,
      .is_register = reg_or_addr.is_register,
      .reg = reg_or_addr.reg,
      .address = reg_or_addr.address,
      .value = value,
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

  if (cmd == "run") {
    return app::Command{
      .type = app::CommandType::Run,
    };
  }

  if (cmd == "b" || cmd == "breakpoint") {
    auto [subcmd, rest] = SplitFirstWhitespace(args);

    if (subcmd == "l" || subcmd == "list") {
      return app::Command{
        .type = app::CommandType::BreakpointList,
      };
    }

    if (subcmd == "a" || subcmd == "a") {
      auto result = ParseAddress(rest);
      if (!result.has_value()) {
        return std::unexpected(ParseError{
          .line = line,
          .message = result.error(),
        });
      }

      return app::Command{
        .type = app::CommandType::BreakpointAdd,
        .address = static_cast<u16>(result.value()),
      };
    }

    if (subcmd == "r" || subcmd == "remove") {
      auto result = ParseAddress(rest);
      if (!result.has_value()) {
        return std::unexpected(ParseError{
          .line = line,
          .message = result.error(),
        });
      }

      return app::Command{
        .type = app::CommandType::BreakpointRemove,
        .address = static_cast<u16>(result.value()),
      };
    }

    return std::unexpected(ParseError{
      .line = line,
      .message = "Expected subcommand one of: l[ist], a[dd], r[emove]",
    });
  }

  return std::unexpected(ParseError{
    .line = line,
    .message = std::format("Unknown command '{}'", cmd),
  });
}
