#pragma once

#include <array>
#include <variant>
#include <optional>
#include <string_view>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/fmt/fmt.h>

#include "opcodes.h"
#include "registers.h"


enum class Cond {
  NZ = 0,
  Z,
  NC,
  C,
};

struct None {};

struct Immediate8 {
  uint8_t value;
};

struct ImmediateS8 {
  int8_t value;
};

struct Immediate16 {
  uint16_t value;
};

struct StackPointer {};

struct Operand {
  using OpType = std::variant<Reg8, Reg16, Cond, Immediate8, Immediate16, ImmediateS8, StackPointer>;

  OpType op;
  bool immediate;
  int8_t offset;

  Operand(OpType o): op{o}, immediate{true}, offset{0} {}
  Operand(OpType o, bool imm): op{o}, immediate{imm}, offset{0} {}
  Operand(OpType o, bool imm, bool incdec): op{o}, immediate{imm}, offset{static_cast<int8_t>(incdec ? 1 : -1)} {}
  Operand(OpType o, bool imm, int8_t off): op{o}, immediate{imm}, offset{off} {}
};

struct Operands {
  Operand dst;
  std::optional<Operand> src;

  Operands(Operand d): dst{d}, src{std::nullopt} {}
  Operands(Operand d, Operand s): dst{d}, src{s} {}
};

struct CycleCount {
  size_t a;
  std::optional<size_t> b;

  CycleCount() = default;
  CycleCount(size_t a_): a{a_}, b{0} {}
  CycleCount(size_t a_, size_t b_): a{a_}, b{b_} {}
};

struct Instruction {
  Opcode opcode;
  size_t bytes;
  CycleCount cycles;
  std::optional<Operands> operands;
};

template <>
struct fmt::formatter<Instruction> {
public:
  constexpr auto parse(fmt::format_parse_context &ctx) {
    return ctx.begin();
  }

  auto format(const Instruction &instr, fmt::format_context &ctx) const {
    return fmt::format_to(ctx.out(), "Instr({})", magic_enum::enum_name(instr.opcode));
  }
};