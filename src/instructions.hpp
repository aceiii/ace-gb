#pragma once

#include <array>
#include <format>
#include <optional>
#include <string_view>
#include <variant>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "opcodes.hpp"
#include "overloaded.h"
#include "registers.hpp"


enum class Cond {
  NZ = 0,
  Z,
  NC,
  C,
};

struct Operands_None {};

struct Operands_SP {};

struct Operands_Cond {
  Cond cond;
};

struct Operands_Reg8 {
  Reg8 reg;
};

struct Operands_Reg16 {
  Reg16 reg;
};

struct Operands_Imm8 {
  uint8_t imm;
};

struct Operands_Imm8_Literal {
  uint8_t imm;
};

struct Operands_Imm8_Literal_Reg8 {
  uint8_t imm;
  Reg8 reg;
};

struct Operands_Imm8_Literal_Reg16_Ptr {
  uint16_t imm;
  Reg16 reg;
};

struct Operands_Offset {
  int8_t offset;
};

struct Operands_Imm16 {
  uint16_t imm;
};

struct Operands_Reg8_Reg8 {
  Reg8 reg1;
  Reg8 reg2;
};

struct Operands_Reg8_Imm8 {
  Reg8 reg;
  uint8_t imm;
};

struct Operands_Reg16_Reg16 {
  Reg16 reg1;
  Reg16 reg2;
};

struct Operands_Reg16_Imm16 {
  Reg16 reg;
  uint16_t imm;
};

struct Operands_Cond_Imm16 {
  Cond cond;
  uint16_t imm;
};

struct Operands_Cond_Offset {
  Cond cond;
  int8_t offset;
};

struct Operands_SP_Reg16 {
  Reg16 reg;
};

struct Operands_SP_Imm16 {
  uint16_t imm;
};

struct Operands_SP_Offset {
  int8_t offset;
};

struct Operands_Imm16_Ptr_Reg8 {
  uint16_t addr;
  Reg8 reg;
};

struct Operands_Imm16_Ptr_SP {
  uint16_t addr;
};

struct Operands_Imm8_Ptr_Reg8 {
  uint8_t addr;
  Reg8 reg;
};

struct Operands_Reg8_Reg8_Ptr {
  Reg8 reg1;
  Reg8 reg2;
};

struct Operands_Reg8_Reg16_Ptr {
  Reg8 reg1;
  Reg16 reg2;
};

struct Operands_Reg8_Reg16_Ptr_Inc {
  Reg8 reg1;
  Reg16 reg2;
};

struct Operands_Reg8_Reg16_Ptr_Dec {
  Reg8 reg1;
  Reg16 reg2;
};

struct Operands_Reg8_Ptr_Reg8 {
  Reg8 reg1;
  Reg8 reg2;
};

struct Operands_Reg8_Imm8_Ptr {
  Reg8 reg;
  uint8_t addr;
};

struct Operands_Reg8_Imm16_Ptr {
  Reg8 reg;
  uint16_t addr;
};

struct Operands_Reg16_SP {
  Reg16 reg;
};

struct Operands_Reg16_SP_Offset {
  Reg16 reg;
  int8_t offset;
};

struct Operands_Reg16_Ptr {
  Reg16 reg;
};

struct Operands_Reg16_Ptr_Reg8 {
  Reg16 reg1;
  Reg8 reg2;
};

struct Operands_Reg16_Ptr_Inc_Reg8 {
  Reg16 reg1;
  Reg8 reg2;
};

struct Operands_Reg16_Ptr_Imm8 {
  Reg16 reg;
  uint8_t imm;
};

struct Operands_Reg16_Ptr_Dec_Reg8 {
  Reg16 reg1;
  Reg8 reg2;
};

typedef std::variant<
    Operands_None,
    Operands_SP,
    Operands_Cond,
    Operands_Reg8,
    Operands_Reg16,
    Operands_Imm8,
    Operands_Imm8_Literal,
    Operands_Imm8_Literal_Reg8,
    Operands_Imm8_Literal_Reg16_Ptr,
    Operands_Imm16,
    Operands_Offset,
    Operands_Reg8_Reg8,
    Operands_Reg8_Imm8,
    Operands_Reg8_Imm8_Ptr,
    Operands_Reg8_Imm16_Ptr,
    Operands_Reg16_Reg16,
    Operands_Reg16_Imm16,
    Operands_Cond_Imm16,
    Operands_Cond_Offset,
    Operands_SP_Reg16,
    Operands_SP_Imm16,
    Operands_SP_Offset,
    Operands_Imm16_Ptr_Reg8,
    Operands_Imm16_Ptr_SP,
    Operands_Imm8_Ptr_Reg8,
    Operands_Reg8_Reg8_Ptr,
    Operands_Reg8_Reg16_Ptr,
    Operands_Reg8_Reg16_Ptr_Inc,
    Operands_Reg8_Reg16_Ptr_Dec,
    Operands_Reg8_Ptr_Reg8,
    Operands_Reg16_SP,
    Operands_Reg16_SP_Offset,
    Operands_Reg16_Ptr,
    Operands_Reg16_Ptr_Reg8,
    Operands_Reg16_Ptr_Inc_Reg8,
    Operands_Reg16_Ptr_Imm8,
    Operands_Reg16_Ptr_Dec_Reg8
> Operands;

struct Instruction {
  Opcode opcode;
  uint8_t bytes;
  uint8_t cycles;
  uint8_t cycles_cond;
  Operands operands;
};

template <>
struct std::formatter<Instruction> {
public:
  constexpr auto parse(auto& ctx) {
    return ctx.begin();
  }

  auto format(const Instruction& instr, auto& ctx) const {
    ZoneScoped;
    return std::format_to(ctx.out(), "Instr({})", magic_enum::enum_name(instr.opcode));
  }
};
