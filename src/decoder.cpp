#include <utility>

#include "decoder.hpp"
#include "instructions.hpp"
#include "opcodes.hpp"
#include "registers.hpp"


namespace Decoder {

Instruction DecodePrefixed(uint8_t op) {
  int r8 = op & 0x7;

  uint8_t bytes = 2;
  uint8_t cycles = r8 == 6 ? 16 : 8;

  if (op & 0xC0) {
    uint8_t b3 = (op & 0x38) >> 3;
    auto operands = ([=]() -> Operands {
      switch (r8) {
      case 0:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::B};
      case 1:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::C};
      case 2:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::D};
      case 3:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::E};
      case 4:
        return Operands_Imm8_Literal_Reg8{b3,Reg8::H};
      case 5:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::L};
      case 6:
        return Operands_Imm8_Literal_Reg16_Ptr{b3, Reg16::HL};
      case 7:
        return Operands_Imm8_Literal_Reg8{b3, Reg8::A};
      default:
        std::unreachable();
      }
    })();

    switch ((op & 0xC0) >> 6) {
    case 1: {
      cycles = r8 == 6 ? 12 : 8;
      return {Opcode::BIT, bytes, cycles, cycles, operands};
    }
    case 2:
      return {Opcode::RES, bytes, cycles, cycles, operands};
    case 3:
      return {Opcode::SET, bytes, cycles, cycles, operands};
    default:
      return {Opcode::INVALID, 1, 4, 4, Operands_None{}};
    }
  }

  auto operands = ([=]() -> Operands {
    switch (r8) {
    case 0:
      return Operands_Reg8{Reg8::B};
    case 1:
      return Operands_Reg8{Reg8::C};
    case 2:
      return Operands_Reg8{Reg8::D};
    case 3:
      return Operands_Reg8{Reg8::E};
    case 4:
      return Operands_Reg8{Reg8::H};
    case 5:
      return Operands_Reg8{Reg8::L};
    case 6:
      return Operands_Reg16_Ptr{Reg16::HL};
    case 7:
      return Operands_Reg8{Reg8::A};
    default:
      std::unreachable();
    }
  })();

  switch ((op & 0xF8) >> 3) {
  case 0:
    return {Opcode::RLC, bytes, cycles, cycles, operands};
  case 1:
    return {Opcode::RRC, bytes, cycles, cycles, operands};
  case 2:
    return {Opcode::RL, bytes, cycles, cycles, operands};
  case 3:
    return {Opcode::RR, bytes, cycles, cycles, operands};
  case 4:
    return {Opcode::SLA, bytes, cycles, cycles, operands};
  case 5:
    return {Opcode::SRA, bytes, cycles, cycles, operands};
  case 6:
    return {Opcode::SWAP, bytes, cycles, cycles, operands};
  case 7:
    return {Opcode::SRL, bytes, cycles, cycles, operands};
  default:
    return {Opcode::INVALID, 1, 4, 4, Operands_None{}};
  }
}

Instruction Decode(uint8_t op) {
  switch (op) {
  case 0x00:
    return {Opcode::NOP, 1, 4, 4, Operands_None{}};
  case 0x01:
    return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{ Reg16::BC }};
  case 0x02:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::BC, Reg8::A}};
  case 0x03:
    return {Opcode::INC, 1, 8, 8, Operands_Reg16{Reg16::BC}};
  case 0x04:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0x05:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0x06:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::B}};
  case 0x07:
    return {Opcode::RLCA, 1, 4, 4, Operands_None{}};
  case 0x08:
    return {Opcode::LD, 3, 20, 20, Operands_Imm16_Ptr_SP{}};
  case 0x09:
    return {Opcode::ADD, 1, 8, 8, Operands_Reg16_Reg16{Reg16::HL, Reg16::BC}};
  case 0x0A:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::BC}};
  case 0x0B:
    return {Opcode::DEC, 1, 8, 8, Operands_Reg16{Reg16::BC}};
  case 0x0C:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0x0D:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0x0E:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::C}};
  case 0x0F:
    return {Opcode::RRCA, 1, 4, 4, Operands_None{}};
  case 0x10:
    return {Opcode::STOP, 2, 4, 4, Operands_None{}};
  case 0x11:
    return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{Reg16::DE}};
  case 0x12:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::DE, Reg8::A}};
  case 0x13:
    return {Opcode::INC, 1, 8, 8, Operands_Reg16{Reg16::DE}};
  case 0x14:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0x15:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0x16:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::D}};
  case 0x17:
    return {Opcode::RLA, 1, 4, 4, Operands_None{}};
  case 0x18:
    return {Opcode::JR, 2, 12, 12, Operands_Offset{}};
  case 0x19:
    return {Opcode::ADD, 1, 8, 8, Operands_Reg16_Reg16{Reg16::HL, Reg16::DE}};
  case 0x1A:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::DE}};
  case 0x1B:
    return {Opcode::DEC, 1, 8, 8, Operands_Reg16{Reg16::DE}};
  case 0x1C:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0x1D:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0x1E:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::E}};
  case 0x1F:
    return {Opcode::RRA, 1, 4, 4, Operands_None{}};
  case 0x20:
    return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::NZ}};
  case 0x21:
    return {Opcode::LD, 3, 12, 12, Operands_Reg16_Imm16{Reg16::HL}};
  case 0x22:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Inc_Reg8{Reg16::HL, Reg8::A}};
  case 0x23:
    return {Opcode::INC, 1, 8, 8, Operands_Reg16{Reg16::HL}};
  case 0x24:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0x25:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0x26:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::H}};
  case 0x27:
    return {Opcode::DAA, 1, 4, 4, Operands_None{}};
  case 0x28:
    return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::Z}};
  case 0x29:
    return {Opcode::ADD, 1, 8, 8, Operands_Reg16_Reg16{Reg16::HL, Reg16::HL}};
  case 0x2A:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr_Inc{Reg8::A, Reg16::HL}};
  case 0x2B:
    return {Opcode::DEC, 1, 8, 8, Operands_Reg16{Reg16::HL}};
  case 0x2C:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::L}};
  case 0x2D:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::L}};
  case 0x2E:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::L}};
  case 0x2F:
    return {Opcode::CPL, 1, 4, 4, Operands_None{}};
  case 0x30:
    return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::NC}};
  case 0x31:
    return {Opcode::LD, 3, 12, 12, Operands_SP_Imm16{}};
  case 0x32:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Dec_Reg8{Reg16::HL, Reg8::A}};
  case 0x33:
    return {Opcode::INC, 1, 8, 8, Operands_SP{}};
  case 0x34:
    return {Opcode::INC, 1, 12, 12, Operands_Reg16_Ptr{Reg16::HL}};
  case 0x35:
    return {Opcode::DEC, 1, 12, 12, Operands_Reg16_Ptr{Reg16::HL}};
  case 0x36:
    return {Opcode::LD, 2, 12, 12, Operands_Reg16_Ptr_Imm8{Reg16::HL}};
  case 0x37:
    return {Opcode::SCF, 1, 4, 4, Operands_None{}};
  case 0x38:
    return {Opcode::JR, 2, 12, 8, Operands_Cond_Offset{Cond::C}};
  case 0x39:
    return {Opcode::ADD, 1, 8, 8, Operands_Reg16_SP{Reg16::HL}};
  case 0x3A:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr_Dec{Reg8::A, Reg16::HL}};
  case 0x3B:
    return {Opcode::DEC, 1, 8, 8, Operands_SP{}};
  case 0x3C:
    return {Opcode::INC, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0x3D:
    return {Opcode::DEC, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0x3E:
    return {Opcode::LD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0x3F:
    return {Opcode::CCF, 1, 4, 4, Operands_None{}};
  case 0x40:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::B}};
  case 0x41:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::C}};
  case 0x42:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::D}};
  case 0x43:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::E}};
  case 0x44:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::H}};
  case 0x45:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::L}};
  case 0x46:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::B, Reg16::HL}};
  case 0x47:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::B, Reg8::A}};
  case 0x48:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::B}};
  case 0x49:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::C}};
  case 0x4A:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::D}};
  case 0x4B:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::E}};
  case 0x4C:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::H}};
  case 0x4D:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::L}};
  case 0x4E:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::C, Reg16::HL}};
  case 0x4F:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::C, Reg8::A}};
  case 0x50:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::B}};
  case 0x51:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::C}};
  case 0x52:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::D}};
  case 0x53:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::E}};
  case 0x54:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::H}};
  case 0x55:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::L}};
  case 0x56:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::D, Reg16::HL}};
  case 0x57:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::D, Reg8::A}};
  case 0x58:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::B}};
  case 0x59:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::C}};
  case 0x5A:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::D}};
  case 0x5B:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::E}};
  case 0x5C:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::H}};
  case 0x5D:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::L}};
  case 0x5E:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::E, Reg16::HL}};
  case 0x5F:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::E, Reg8::A}};
  case 0x60:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::B}};
  case 0x61:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::C}};
  case 0x62:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::D}};
  case 0x63:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::E}};
  case 0x64:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::H}};
  case 0x65:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::L}};
  case 0x66:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::H, Reg16::HL}};
  case 0x67:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::H, Reg8::A}};
  case 0x68:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::B}};
  case 0x69:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::C}};
  case 0x6A:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::D}};
  case 0x6B:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::E}};
  case 0x6C:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::H}};
  case 0x6D:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::L}};
  case 0x6E:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::L, Reg16::HL}};
  case 0x6F:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::L, Reg8::A}};
  case 0x70:
    return {Opcode::LD, 1, 8, 4, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::B}};
  case 0x71:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::C}};
  case 0x72:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::D}};
  case 0x73:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::E}};
  case 0x74:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::H}};
  case 0x75:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::L}};
  case 0x76:
    return {Opcode::HALT, 1, 4, 4, Operands_None{}};
  case 0x77:
    return {Opcode::LD, 1, 8, 8, Operands_Reg16_Ptr_Reg8{Reg16::HL, Reg8::A}};
  case 0x78:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::B}};
  case 0x79:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::C}};
  case 0x7A:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::D}};
  case 0x7B:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::E}};
  case 0x7C:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::H}};
  case 0x7D:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::L}};
  case 0x7E:
    return {Opcode::LD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::HL}};
  case 0x7F:
    return {Opcode::LD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::A}};
  case 0x80:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::B}};
  case 0x81:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::C}};
  case 0x82:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::D}};
  case 0x83:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::E}};
  case 0x84:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::H}};
  case 0x85:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::L}};
  case 0x86:
    return {Opcode::ADD, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::HL}};
  case 0x87:
    return {Opcode::ADD, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::A}};
  case 0x88:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::B}};
  case 0x89:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::C}};
  case 0x8A:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::D}};
  case 0x8B:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::E}};
  case 0x8C:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::H}};
  case 0x8D:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::L}};
  case 0x8E:
    return {Opcode::ADC, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::HL}};
  case 0x8F:
    return {Opcode::ADC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::A}};
  case 0x90:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0x91:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0x92:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0x93:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0x94:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0x95:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::L}};
  case 0x96:
    return {Opcode::SUB, 1, 8, 8, Operands_Reg16_Ptr{Reg16::HL}};
  case 0x97:
    return {Opcode::SUB, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0x98:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::B}};
  case 0x99:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::C}};
  case 0x9A:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::D}};
  case 0x9B:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::E}};
  case 0x9C:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::H}};
  case 0x9D:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::L}};
  case 0x9E:
    return {Opcode::SBC, 1, 8, 8, Operands_Reg8_Reg16_Ptr{Reg8::A, Reg16::HL}};
  case 0x9F:
    return {Opcode::SBC, 1, 4, 4, Operands_Reg8_Reg8{Reg8::A, Reg8::A}};
  case 0xA0:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0xA1:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0xA2:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0xA3:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0xA4:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0xA5:
    return {Opcode::AND, 1, 4,4, Operands_Reg8{Reg8::L}};
  case 0xA6:
    return {Opcode::AND, 1, 8, 8, Operands_Reg16_Ptr{Reg16::HL}};
  case 0xA7:
    return {Opcode::AND, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0xA8:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0xA9:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0xAA:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0xAB:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0xAC:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0xAD:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::L}};
  case 0xAE:
    return {Opcode::XOR, 1, 8, 8, Operands_Reg16_Ptr{Reg16::HL}};
  case 0xAF:
    return {Opcode::XOR, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0xB0:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0xB1:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0xB2:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0xB3:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0xB4:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0xB5:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::L}};
  case 0xB6:
    return {Opcode::OR, 1, 8, 8, Operands_Reg16_Ptr{Reg16::HL}};
  case 0xB7:
    return {Opcode::OR, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0xB8:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::B}};
  case 0xB9:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::C}};
  case 0xBA:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::D}};
  case 0xBB:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::E}};
  case 0xBC:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::H}};
  case 0xBD:
    return {Opcode::CP, 1, 4, 4,Operands_Reg8{Reg8::L}};
  case 0xBE:
    return {Opcode::CP, 1, 8, 8, Operands_Reg16_Ptr{Reg16::HL}};
  case 0xBF:
    return {Opcode::CP, 1, 4, 4, Operands_Reg8{Reg8::A}};
  case 0xC0:
    return {Opcode::RET, 1, 20, 8, Operands_Cond{Cond::NZ}};
  case 0xC1:
    return {Opcode::POP, 1, 12, 12, Operands_Reg16{Reg16::BC}};
  case 0xC2:
    return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::NZ}};
  case 0xC3:
    return {Opcode::JP, 3, 16, 16, Operands_Imm16{}};
  case 0xC4:
    return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::NZ}};
  case 0xC5:
    return {Opcode::PUSH, 1, 16, 16, Operands_Reg16{Reg16::BC}};
  case 0xC6:
    return {Opcode::ADD, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xC7:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x00}};
  case 0xC8:
    return {Opcode::RET, 1, 20, 8, Operands_Cond{Cond::Z}};
  case 0xC9:
    return {Opcode::RET, 1, 16, 16, Operands_None{}};
  case 0xCA:
    return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::Z}};
  case 0xCB:
    return {Opcode::PREFIX, 1, 4, 4, Operands_None{}};
  case 0xCC:
    return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::Z}};
  case 0xCD:
    return {Opcode::CALL, 3, 24, 24, Operands_Imm16{}};
  case 0xCE:
    return {Opcode::ADC, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xCF:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x08}};
  case 0xD0:
    return {Opcode::RET, 1, 20, 8, Operands_Cond{Cond::NC}};
  case 0xD1:
    return {Opcode::POP, 1, 12, 12, Operands_Reg16{Reg16::DE}};
  case 0xD2:
    return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::NC}};
  case 0xD4:
    return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::NC}};
  case 0xD5:
    return {Opcode::PUSH, 1, 16, 16, Operands_Reg16{Reg16::DE}};
  case 0xD6:
    return {Opcode::SUB, 2, 8, 8, Operands_Imm8{}};
  case 0xD7:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x10}};
  case 0xD8:
    return {Opcode::RET, 1, 20, 8, Operands_Cond{Cond::C}};
  case 0xD9:
    return {Opcode::RETI, 1, 16, 16, Operands_None{}};
  case 0xDA:
    return {Opcode::JP, 3, 16, 12, Operands_Cond_Imm16{Cond::C}};
  case 0xDC:
    return {Opcode::CALL, 3, 24, 12, Operands_Cond_Imm16{Cond::C}};
  case 0xDE:
    return {Opcode::SBC, 2, 8, 8, Operands_Reg8_Imm8{Reg8::A}};
  case 0xDF:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x18}};
  case 0xE0:
    return {Opcode::LDH, 2, 12, 12, Operands_Imm8_Ptr_Reg8{0, Reg8::A}};
  case 0xE1:
    return {Opcode::POP, 1, 12, 12, Operands_Reg16{Reg16::HL}};
  case 0xE2:
    return {Opcode::LDH, 1, 8, 8, Operands_Reg8_Ptr_Reg8{Reg8::C, Reg8::A}};
  case 0xE5:
    return {Opcode::PUSH, 1, 16, 16, Operands_Reg16{Reg16::HL}};
  case 0xE6:
    return {Opcode::AND, 2, 8, 8, Operands_Imm8{}};
  case 0xE7:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x20}};
  case 0xE8:
    return {Opcode::ADD, 2, 16, 16, Operands_SP_Offset{}};
  case 0xE9:
    return {Opcode::JP, 1, 4, 4, Operands_Reg16{Reg16::HL}};
  case 0xEA:
    return {Opcode::LD, 3, 16, 16, Operands_Imm16_Ptr_Reg8{0, Reg8::A}};
  case 0xEE:
    return {Opcode::XOR, 2, 8, 8, Operands_Imm8{}};
  case 0xEF:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x28}};
  case 0xF0:
    return {Opcode::LDH, 2, 12, 12, Operands_Reg8_Imm8_Ptr{Reg8::A}};
  case 0xF1:
    return {Opcode::POP, 1, 12, 12, Operands_Reg16{Reg16::AF}};
  case 0xF2:
    return {Opcode::LDH, 1, 8, 8, Operands_Reg8_Reg8_Ptr{Reg8::A, Reg8::C}};
  case 0xF3:
    return {Opcode::DI, 1, 4, 4, Operands_None{}};
  case 0xF5:
    return {Opcode::PUSH, 1, 16, 16, Operands_Reg16{Reg16::AF}};
  case 0xF6:
    return {Opcode::OR, 2, 8, 8, Operands_Imm8{}};
  case 0xF7:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x30}};
  case 0xF8:
    return {Opcode::LD, 2, 12, 12, Operands_Reg16_SP_Offset{Reg16::HL}};
  case 0xF9:
    return {Opcode::LD, 1, 8, 8, Operands_SP_Reg16{Reg16::HL}};
  case 0xFA:
    return {Opcode::LD, 3, 16, 16, Operands_Reg8_Imm16_Ptr{Reg8::A, 0}};
  case 0xFB:
    return {Opcode::EI, 1, 4, 4, Operands_None{}};
  case 0xFE:
    return {Opcode::CP, 2, 8, 8, Operands_Imm8{}};
  case 0xFF:
    return {Opcode::RST, 1, 16, 16, Operands_Imm8_Literal{0x38}};
  default:
    return {Opcode::INVALID, 1, 4, 4, Operands_None{}};
  }
}

}
