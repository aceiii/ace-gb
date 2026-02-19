#include <cassert>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "cpu.hpp"
#include "opcodes.h"
#include "overloaded.h"
#include "instructions.hpp"


inline uint16_t interrupt_handler(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank: return 0x40;
    case Interrupt::Stat: return 0x48;
    case Interrupt::Timer: return 0x50;
    case Interrupt::Serial: return 0x58;
    case Interrupt::Joypad: return 0x60;
    default: std::unreachable();
  }
}

inline bool check_cond(Cpu &cpu, Cond cond) {
  switch (cond) {
  case Cond::NZ: return !cpu.regs.get(Flag::Z);
  case Cond::Z: return cpu.regs.get(Flag::Z);
  case Cond::NC: return !cpu.regs.get(Flag::C);
  case Cond::C: return cpu.regs.get(Flag::C);
  default: std::unreachable();
  }
}

inline void instr_load_reg8_reg8(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.regs.at(r1) = cpu.regs.get(r2);
}

inline void instr_load_reg8_imm8(Cpu &cpu, Reg8 r1, uint8_t imm) {
  cpu.regs.at(r1) = imm;
}

inline void instr_load_reg8_reg16_ptr(Cpu &cpu, Reg8 r1, Reg16 r2) {
  cpu.regs.at(r1) = cpu.Read8(cpu.regs.get(r2));
}

inline void instr_load_reg16_ptr_reg8(Cpu &cpu, Reg16 r1, Reg8 r2) {
  cpu.Write8(cpu.regs.get(r1), cpu.regs.get(r2));
}

inline void instr_load_reg16_ptr_imm8(Cpu &cpu, Reg16 r1, uint8_t val) {
  cpu.Write8(cpu.regs.get(r1), val);
}

inline void instr_load_reg8_imm16_ptr(Cpu &cpu, Reg8 r1, uint16_t val) {
  cpu.regs.at(r1) = cpu.Read8(val);
}

inline void instr_load_imm16_ptr_reg8(Cpu &cpu, uint16_t val, Reg8 r1) {
  cpu.Write8(val, cpu.regs.get(r1));
}

inline void instr_load_hi_reg8_reg8_ptr(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.regs.at(r1) = cpu.Read8(0xff00 | cpu.regs.get(r2));
}

inline void instr_load_hi_reg8_ptr_reg8(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.Write8(0xff00 | cpu.regs.get(r1), cpu.regs.get(r2));
}

inline void instr_load_hi_reg8_imm8_ptr(Cpu &cpu, Reg8 r1, uint8_t val) {
  cpu.regs.at(r1) = cpu.Read8(0xff00 | val);
}

inline void instr_load_hi_imm8_ptr_reg8(Cpu &cpu, uint8_t val, Reg8 r1) {
  cpu.Write8(0xff00 | val, cpu.regs.get(r1));
}

inline void instr_load_reg8_reg16_ptr_dec(Cpu &cpu, Reg8 r1, Reg16 r2) {
  auto addr = cpu.regs.get(r2);
  cpu.regs.at(r1) = cpu.Read8(addr);
  cpu.regs.set(r2, addr - 1);
}

inline void instr_load_reg16_ptr_dec_reg8(Cpu &cpu, Reg16 r1, Reg8 r2) {
  auto addr = cpu.regs.get(r1);
  cpu.Write8(addr, cpu.regs.get(r2));
  cpu.regs.set(r1, addr - 1);
}

inline void instr_load_reg8_reg16_ptr_inc(Cpu &cpu, Reg8 r1, Reg16 r2) {
  auto addr = cpu.regs.get(r2);
  cpu.regs.at(r1) = cpu.Read8(addr);
  cpu.regs.set(r2, addr + 1);
}

inline void instr_load_reg16_ptr_inc_reg8(Cpu &cpu, Reg16 r1, Reg8 r2) {
  auto addr = cpu.regs.get(r1);
  cpu.Write8(addr, cpu.regs.get(r2));
  cpu.regs.set(r1, addr + 1);
}

inline void instr_load_reg16_reg16(Cpu &cpu, Reg16 r1, Reg16 r2) {
  cpu.regs.set(r1, cpu.regs.get(r2));
  cpu.Tick();
}

inline void instr_load_reg16_imm16(Cpu &cpu, Reg16 r1, uint16_t val) {
  cpu.regs.set(r1, val);
}

inline void instr_load_imm16_ptr_sp(Cpu &cpu, uint16_t val) {
  cpu.Write16(val, cpu.regs.sp);
}

inline void instr_load_sp_reg16(Cpu &cpu, Reg16 r1) {
  cpu.regs.sp = cpu.regs.get(r1);
  cpu.Tick();
}

inline void instr_load_sp_imm16(Cpu &cpu, uint16_t imm) {
  cpu.regs.sp = imm;
}

inline void instr_load_reg16_sp_offset(Cpu &cpu, Reg16 r1, int8_t e) {
  uint32_t sp = cpu.regs.sp;
  uint32_t result = sp + e;

  cpu.regs.set(r1, result);
  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  cpu.regs.set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);
  cpu.Tick();
}

inline void instr_push_reg16(Cpu &cpu, Reg16 r1) {
  uint16_t val = cpu.regs.get(r1);
  cpu.Push16(val);
}

inline void instr_pop_reg16(Cpu &cpu, Reg16 r1) {
  auto val = cpu.Pop16();
  cpu.regs.set(r1, val);
}

inline uint8_t instr_add8(Cpu &cpu, uint8_t a, uint8_t b, uint8_t c) {
  uint8_t result_half = (a & 0xf) + (b &0xf) + c;
  uint16_t result = a + b + c;

  cpu.regs.set(Flag::Z, (result & 0xff) == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (result_half >> 4) & 0x1);
  cpu.regs.set(Flag::C, (result >> 8) & 0x1);

  return (uint8_t)result;
}

inline uint16_t instr_add16(Cpu &cpu, uint16_t a, uint16_t b) {
  uint16_t result_half = (a & 0xfff) + (b & 0xfff);
  uint32_t result = a + b;

  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (result_half >> 12) & 0x1);
  cpu.regs.set(Flag::C, (result >> 16) & 0x1);

  return result;
}

inline void instr_add_reg8(Cpu &cpu, Reg8 r) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), cpu.regs.get(r), 0);
}

inline void instr_add_reg16_ptr(Cpu &cpu, Reg16 r) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), cpu.Read8(cpu.regs.get(r)), 0);
}

inline void instr_add_imm8(Cpu &cpu, uint8_t imm) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), imm, 0);
}

inline void instr_add_reg8_reg8(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.regs.at(r1) = instr_add8(cpu, cpu.regs.get(r1), cpu.regs.get(r2), 0);
}

inline void instr_add_reg8_imm8(Cpu &cpu, Reg8 r, uint8_t imm) {
  cpu.regs.at(r) = instr_add8(cpu, cpu.regs.get(r), imm, 0);
}

inline void instr_add_reg8_reg16_ptr(Cpu &cpu, Reg8 r1, Reg16 r2) {
  cpu.regs.at(r1) = instr_add8(cpu, cpu.regs.get(r1), cpu.Read8(cpu.regs.get(r2)), 0);
}

inline void instr_add_carry_reg8(Cpu &cpu, Reg8 r) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), cpu.regs.get(r), cpu.regs.get(Flag::C));
}

inline void instr_add_carry_reg8_reg8(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.regs.at(r1) = instr_add8(cpu, cpu.regs.get(r1), cpu.regs.get(r2), cpu.regs.get(Flag::C));
}

inline void instr_add_carry_reg8_imm8(Cpu &cpu, Reg8 r, uint8_t imm) {
  cpu.regs.at(r) = instr_add8(cpu, cpu.regs.get(r), imm, cpu.regs.get(Flag::C));
}

inline void instr_add_carry_reg8_reg16_ptr(Cpu &cpu, Reg8 r1, Reg16 r2) {
  cpu.regs.at(r1) = instr_add8(cpu, cpu.regs.get(r1), cpu.Read8(cpu.regs.get(r2)), cpu.regs.get(Flag::C));
}

inline void instr_add_carry_reg16_ptr(Cpu &cpu, Reg16 r) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), cpu.Read8(cpu.regs.get(r)), cpu.regs.get(Flag::C));
}

inline void instr_add_carry_imm8(Cpu &cpu, uint8_t imm) {
  cpu.regs.at(Reg8::A) = instr_add8(cpu, cpu.regs.get(Reg8::A), imm, cpu.regs.get(Flag::C));
}

inline void instr_add_reg16_sp(Cpu &cpu, Reg16 r) {
  cpu.regs.set(r, instr_add16(cpu, cpu.regs.get(r), cpu.regs.sp));
  cpu.Tick();
}

inline void instr_add_reg16_reg16(Cpu &cpu, Reg16 r1, Reg16 r2) {
  cpu.regs.set(r1, instr_add16(cpu, cpu.regs.get(r1), cpu.regs.get(r2)));
  cpu.Tick();
}

inline void instr_add_sp_offset(Cpu &cpu, int8_t e) {
  uint32_t sp = cpu.regs.sp;
  uint32_t result = sp + e;

  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  cpu.regs.set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);

  cpu.regs.sp = result;
  cpu.Tick();
  cpu.Tick();
}

inline uint8_t instr_sub8(Cpu &cpu, uint8_t a, uint8_t b, uint8_t c) {
  auto half_result = static_cast<int16_t>(a & 0xf) - (b & 0xf) - c;
  auto result = static_cast<int16_t>(a) - b - c;

  cpu.regs.set(Flag::Z, (uint8_t)result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 1);
  cpu.regs.set(Flag::H, half_result < 0 ? 1 : 0);
  cpu.regs.set(Flag::C, result < 0 ? 1 : 0);

  return result;
}

inline void instr_sub_reg8(Cpu &cpu, Reg8 r) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.regs.get(r), 0);
}

inline void instr_sub_reg16_ptr(Cpu &cpu, Reg16 r) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.Read8(cpu.regs.get(r)), 0);
}

inline void instr_sub_imm8(Cpu &cpu, uint8_t imm) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), imm, 0);
}

inline void instr_sub_carry_reg8(Cpu &cpu, Reg8 r) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.regs.get(r), 0);
}

inline void instr_sub_carry_reg16_ptr(Cpu &cpu, Reg16 r) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.Read8(cpu.regs.get(r)), cpu.regs.get(Flag::C));
}

inline void instr_sub_carry_imm8(Cpu &cpu, uint8_t imm) {
  cpu.regs.at(Reg8::A) = instr_sub8(cpu, cpu.regs.get(Reg8::A), imm, cpu.regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_reg8(Cpu &cpu, Reg8 r1, Reg8 r2) {
  cpu.regs.at(r1) = instr_sub8(cpu, cpu.regs.get(r1), cpu.regs.get(r2), cpu.regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_imm8(Cpu &cpu, Reg8 r, uint8_t imm) {
  cpu.regs.at(r) = instr_sub8(cpu, cpu.regs.get(r), imm, cpu.regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_reg16_ptr(Cpu &cpu, Reg8 r1, Reg16 r2) {
  cpu.regs.at(r1) = instr_sub8(cpu, cpu.regs.get(r1), cpu.Read8(cpu.regs.get(r2)), cpu.regs.get(Flag::C));
}

inline void instr_cmp_reg8(Cpu &cpu, Reg8 r) {
  instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.regs.get(r), 0);
}

inline void instr_cmp_reg16_ptr(Cpu &cpu, Reg16 r) {
  instr_sub8(cpu, cpu.regs.get(Reg8::A), cpu.Read8(cpu.regs.get(r)), 0);
}

inline void instr_cmp_imm8(Cpu &cpu, uint8_t imm) {
  instr_sub8(cpu, cpu.regs.get(Reg8::A), imm, 0);
}

inline void instr_inc_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t result = val + 1;
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t result = val + 1;
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_sp(Cpu &cpu) {
  cpu.regs.sp += 1;
  cpu.Tick();
}

inline void instr_inc_reg16(Cpu &cpu, Reg16 r) {
  cpu.regs.set(r, cpu.regs.get(r) + 1);
  cpu.Tick();
}

inline void instr_dec_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t result = val - 1;
  cpu.regs.at(r) = result;
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 1);
  cpu.regs.set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t result = val - 1;
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 1);
  cpu.regs.set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_sp(Cpu &cpu) {
  cpu.regs.sp -= 1;
  cpu.Tick();
}

inline void instr_dec_reg16(Cpu &cpu, Reg16 r) {
  cpu.regs.set(r, cpu.regs.get(r) - 1);
  cpu.Tick();
}

inline void instr_and_reg8(Cpu &cpu, Reg8 r) {
  auto result = cpu.regs.at(Reg8::A) &= cpu.regs.get(r);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 1);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_and_reg16_ptr(Cpu &cpu, Reg16 r) {
  auto result = cpu.regs.at(Reg8::A) &= cpu.Read8(cpu.regs.get(r));
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 1);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_and_imm8(Cpu &cpu, uint8_t imm) {
  auto result = cpu.regs.at(Reg8::A) &= imm;
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 1);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_or_reg8(Cpu &cpu, Reg8 r) {
  auto result = cpu.regs.at(Reg8::A) |= cpu.regs.get(r);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_or_reg16_ptr(Cpu &cpu, Reg16 r) {
  auto result = cpu.regs.at(Reg8::A) |= cpu.Read8(cpu.regs.get(r));
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_or_imm8(Cpu &cpu, uint8_t imm) {
  auto result = cpu.regs.at(Reg8::A) |= imm;
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_xor_reg8(Cpu &cpu, Reg8 r) {
  auto result = cpu.regs.at(Reg8::A) ^= cpu.regs.get(r);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_xor_reg16_ptr(Cpu &cpu, Reg16 r) {
  auto result = cpu.regs.at(Reg8::A) ^= cpu.Read8(cpu.regs.get(r));
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_xor_imm8(Cpu &cpu, uint8_t imm) {
  auto result = cpu.regs.at(Reg8::A) ^= imm;
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_ccf(Cpu &cpu) {
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, cpu.regs.get(Flag::C) ? 0 : 1);
}

inline void instr_scf(Cpu &cpu) {
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 1);
}

inline void instr_daa(Cpu &cpu) {
  auto a = cpu.regs.get(Reg8::A);
  auto n = cpu.regs.get(Flag::N);
  auto h = cpu.regs.get(Flag::H);
  auto c = cpu.regs.get(Flag::C);

  if (n) {
    if (c) {
      a -= 0x60;
    }
    if (h) {
      a -= 0x6;
    }
  } else {
    if (c || a > 0x99) {
      a += 0x60;
      cpu.regs.set(Flag::C, 1);
    }
    if (h || (a & 0x0f) > 0x09) {
      a += 0x6;
    }
  }

  cpu.regs.set(Reg8::A, a);
  cpu.regs.set(Flag::Z, a == 0 ? 1 : 0);
  cpu.regs.set(Flag::H, 0);
}

inline void instr_cpl(Cpu &cpu) {
  cpu.regs.set(Reg8::A, ~cpu.regs.get(Reg8::A));
  cpu.regs.set(Flag::N, 1);
  cpu.regs.set(Flag::H, 1);
}

inline void instr_rlca(Cpu &cpu) {
  uint8_t val = cpu.regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  cpu.regs.set(Reg8::A, result);
  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rrca(Cpu &cpu) {
  uint8_t val = cpu.regs.get(Reg8::A);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  cpu.regs.set(Reg8::A, result);
  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rla(Cpu &cpu) {
  uint8_t val = cpu.regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | cpu.regs.get(Flag::C);
  cpu.regs.set(Reg8::A, result);
  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C,  carry);
}

inline void instr_rra(Cpu &cpu) {
  uint8_t val = cpu.regs.get(Reg8::A);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (cpu.regs.get(Flag::C) << 7);
  cpu.regs.set(Reg8::A, result);
  cpu.regs.set(Flag::Z, 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rlc_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rlc_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rrc_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rrc_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rl_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | cpu.regs.get(Flag::C);
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C,  carry);
}

inline void instr_rl_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | cpu.regs.get(Flag::C);
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C,  carry);
}

inline void instr_rr_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (cpu.regs.get(Flag::C) << 7);
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_rr_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (cpu.regs.get(Flag::C) << 7);
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_sla_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = val << 1;
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_sla_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = val << 1;
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_srl_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = val >> 1;
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_srl_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = val >> 1;
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_sra_reg8(Cpu &cpu, Reg8 r) {
  uint8_t val = cpu.regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (val & 0x80);
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_sra_reg16_ptr(Cpu &cpu, Reg16 r) {
  uint8_t val = cpu.Read8(cpu.regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (val & 0x80);
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, carry);
}

inline void instr_swap_reg8(Cpu &cpu, Reg8 r) {
  auto val = cpu.regs.get(r);
  uint8_t upper = (val >> 4) & 0xf;
  uint8_t lower = val & 0xf;
  uint8_t result = upper | (lower << 4);
  cpu.regs.set(r, result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_swap_reg16_ptr(Cpu &cpu, Reg16 r) {
  auto val = cpu.Read8(cpu.regs.get(r));
  uint8_t upper = (val >> 4) & 0xf;
  uint8_t lower = val & 0xf;
  uint8_t result = upper | (lower << 4);
  cpu.Write8(cpu.regs.get(r), result);
  cpu.regs.set(Flag::Z, result == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 0);
  cpu.regs.set(Flag::C, 0);
}

inline void instr_bit_imm8_reg8(Cpu &cpu, uint8_t imm, Reg8 r) {
  auto bit = (cpu.regs.get(r) >> imm) & 0x1;
  cpu.regs.set(Flag::Z, bit == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 1);
}

inline void instr_bit_imm8_reg16_ptr(Cpu &cpu, uint8_t imm, Reg16 r) {
  auto bit = (cpu.Read8(cpu.regs.get(r)) >> imm) & 0x1;
  cpu.regs.set(Flag::Z, bit == 0 ? 1 : 0);
  cpu.regs.set(Flag::N, 0);
  cpu.regs.set(Flag::H, 1);
}

inline void instr_res_imm8_reg8(Cpu &cpu, uint8_t imm, Reg8 r) {
  uint8_t mask = cpu.regs.get(r) & (1 << imm);
  cpu.regs.at(r) ^= mask;
}

inline void instr_res_imm8_reg16_ptr(Cpu &cpu, uint8_t imm, Reg16 r) {
  uint16_t addr = cpu.regs.get(r);
  uint8_t val = cpu.Read8(addr);
  uint8_t mask = val & (1 << imm);
  uint8_t result = val ^ mask;
  cpu.Write8(addr, result);
}

inline void instr_set_imm8_reg8(Cpu &cpu, uint8_t imm, Reg8 r) {
  uint8_t mask = 1 << imm;
  cpu.regs.at(r) |= mask;
}

inline void instr_set_imm8_reg16_ptr(Cpu &cpu, uint8_t imm, Reg16 r) {
  uint8_t mask = 1 << imm;
  uint16_t addr = cpu.regs.get(r);
  uint8_t result = cpu.Read8(addr) | mask;
  cpu.Write8(addr, result);
}

inline bool instr_jump_imm16(Cpu &cpu, uint16_t imm) {
  cpu.regs.pc = imm;
  cpu.Tick();
  return true;
}

inline bool instr_jump_reg16(Cpu &cpu, Reg16 r) {
  cpu.regs.pc = cpu.regs.get(r);
  return true;
}

inline bool instr_jump_cond_imm16(Cpu &cpu, Cond cond, uint16_t nn) {
  if (check_cond(cpu, cond)) {
    cpu.regs.pc = nn;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_jump_rel(Cpu &cpu, int8_t e) {
  cpu.regs.pc += e;
  cpu.Tick();
  return true;
}

inline bool instr_jump_rel_cond(Cpu &cpu, Cond cond, int8_t e) {
  if (check_cond(cpu, cond)) {
    cpu.regs.pc += e;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_call_imm16(Cpu &cpu, uint16_t nn) {
  cpu.Push16(cpu.regs.pc);
  cpu.regs.pc = nn;
  cpu.Tick();
  return true;
}

inline bool instr_call_cond_imm16(Cpu &cpu, Cond cond, uint16_t nn) {
  if (check_cond(cpu, cond)) {
    cpu.Push16(cpu.regs.pc);
    cpu.regs.pc = nn;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_ret(Cpu &cpu) {
  cpu.regs.pc = cpu.Pop16();
  cpu.Tick();
  return true;
}

inline bool instr_ret_cond(Cpu &cpu, Cond cond) {
  auto jump = check_cond(cpu, cond);
  cpu.Tick();
  if (jump) {
    cpu.regs.pc = cpu.Pop16();
    cpu.Tick();
    return true;
  }
  return false;
}

inline void instr_reti(Cpu &cpu) {
  cpu.regs.pc = cpu.Pop16();
  cpu.state.ime = true;
  cpu.Tick();
}

inline void instr_restart(Cpu &cpu, uint8_t n) {
  cpu.Push16(cpu.regs.pc);
  cpu.regs.pc = n;
  cpu.Tick();
}

inline void execute_ld(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_SP_Reg16 &operands) { instr_load_sp_reg16(cpu, operands.reg); },
     [&](Operands_SP_Imm16 &operands) { instr_load_sp_imm16(cpu, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_load_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_load_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg16_Reg16 &operands) { instr_load_reg16_reg16(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Imm16 &operands) { instr_load_reg16_imm16(cpu, operands.reg, operands.imm); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_load_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Inc &operands) { instr_load_reg8_reg16_ptr_inc(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Dec &operands) { instr_load_reg8_reg16_ptr_dec(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { instr_load_reg8_imm16_ptr(cpu, operands.reg, operands.addr); },
     [&](Operands_Reg16_SP_Offset &operands) { instr_load_reg16_sp_offset(cpu, operands.reg, operands.offset); },
     [&](Operands_Reg16_Ptr_Reg8 &operands) { instr_load_reg16_ptr_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Inc_Reg8 &operands) { instr_load_reg16_ptr_inc_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { instr_load_reg16_ptr_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg16_Ptr_Dec_Reg8 &operands) { instr_load_reg16_ptr_dec_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { instr_load_imm16_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Imm16_Ptr_SP &operands) { instr_load_imm16_ptr_sp(cpu, operands.addr); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

inline void execute_inc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_inc_reg8(cpu, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_inc_reg16(cpu, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_inc_reg16_ptr(cpu, operands.reg); },
     [&](Operands_SP &operands) { instr_inc_sp(cpu); },
     [&](auto&) { std::unreachable(); },
  }, instr.operands);
}

void execute_dec(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_dec_reg8(cpu, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_dec_reg16(cpu, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_dec_reg16_ptr(cpu, operands.reg); },
     [&](Operands_SP &operands) { instr_dec_sp(cpu); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_add(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_imm8(cpu, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_add_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Reg16 &operands) { instr_add_reg16_reg16(cpu, operands.reg1, operands.reg2); },
     [&](Operands_SP_Offset &operands) { instr_add_sp_offset(cpu, operands.offset); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg16_SP &operands) { instr_add_reg16_sp(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_adc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_carry_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_carry_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_carry_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_carry_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_add_carry_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_carry_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sub(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sbc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_carry_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_carry_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_carry_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_sub_carry_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_sub_carry_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_sub_carry_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_and(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_and_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_and_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_and_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_xor(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_xor_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_xor_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_xor_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_or(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_or_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_or_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_or_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_cp(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_cmp_reg8(cpu, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_cmp_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_cmp_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_rlca(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rlca(cpu);
}

void execute_rrca(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rrca(cpu);
}

void execute_rla(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rla(cpu);
}

void execute_rra(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rra(cpu);
}

void execute_daa(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_daa(cpu);
}

void execute_cpl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_cpl(cpu);
}

void execute_scf(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_scf(cpu);
}

void execute_ccf(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_ccf(cpu);
}

bool execute_jr(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  return std::visit(overloaded{
     [&](Operands_Offset &operands) { return instr_jump_rel(cpu, operands.offset); },
     [&](Operands_Cond_Offset &operands) { return instr_jump_rel_cond(cpu, operands.cond, operands.offset); },
     [&](auto &) { std::unreachable(); return false; },
  }, instr.operands);
}

void execute_halt(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.halt = true;
}

void execute_ldh(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8_Imm8_Ptr &operands) { instr_load_hi_reg8_imm8_ptr(cpu, operands.reg, operands.addr); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg8_Ptr &operands) { instr_load_hi_reg8_reg8_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Ptr_Reg8 &operands) { instr_load_hi_reg8_ptr_reg8(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_pop(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_pop_reg16(cpu, operands.reg);
}

void execute_push(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.Tick();
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_push_reg16(cpu, operands.reg);
}

void execute_rst(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  auto operands = std::get<Operands_Imm8_Literal>(instr.operands);
  instr_restart(cpu, operands.imm);
}

bool execute_call(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm16>(&instr.operands)) {
    return instr_call_imm16(cpu, ops->imm);
  } else if (const auto *ops = std::get_if<Operands_Cond_Imm16>(&instr.operands)) {
    return instr_call_cond_imm16(cpu, ops->cond, ops->imm);
  }
  std::unreachable();
}

bool execute_jp(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  return std::visit(overloaded{
    [&](Operands_Reg16 &operands) { return instr_jump_reg16(cpu, operands.reg); },
    [&](Operands_Imm16 &operands) { return instr_jump_imm16(cpu, operands.imm); },
    [&](Operands_Cond_Imm16 &operands) { return instr_jump_cond_imm16(cpu, operands.cond, operands.imm); },
    [&](auto &) { std::unreachable(); return false; },
  }, instr.operands);
}

bool execute_ret(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (std::get_if<Operands_None>(&instr.operands)) {
    return instr_ret(cpu);
  } else if (const auto *ops = std::get_if<Operands_Cond>(&instr.operands)) {
    return instr_ret_cond(cpu, ops->cond);
  }
  std::unreachable();
}

void execute_reti(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_reti(cpu);
}

void execute_rlc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rlc_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rlc_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rrc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rrc_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rrc_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rl_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rl_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rr(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rr_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rr_reg16_ptr(cpu, ops->reg);
  }
}

void execute_sla(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sla_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sla_reg16_ptr(cpu, ops->reg);
  }
}

void execute_sra(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sra_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sra_reg16_ptr(cpu, ops->reg);
  }
}

void execute_srl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_srl_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_srl_reg16_ptr(cpu, ops->reg);
  }
}

void execute_swap(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_swap_reg8(cpu,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_swap_reg16_ptr(cpu, ops->reg);
  }
}

void execute_bit(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_bit_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_bit_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_res(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_res_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_res_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_set(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_set_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_set_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_di(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.ime = false;
}

void execute_ei(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.ime = true;
}

void execute_stop(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.stop = true;
  cpu.Write8(std::to_underlying(IO::DIV), 0);
}

uint8_t Cpu::Execute() {
  tick_counter = 0;

  auto interrupt_cycles = ExecuteInterrupts();
  if (interrupt_cycles) {
    return interrupt_cycles;
  }

  if (state.halt) {
    Tick();
    return 4;
  }

  auto logger = spdlog::get("doctor_logger");
  if (logger) {
    auto a = regs.get(Reg8::A);
    auto f = regs.get(Reg8::F);
    auto b = regs.get(Reg8::B);
    auto c = regs.get(Reg8::C);
    auto d = regs.get(Reg8::D);
    auto e = regs.get(Reg8::E);
    auto h = regs.get(Reg8::H);
    auto l = regs.get(Reg8::L);
    auto pc = regs.pc;
    auto sp = regs.sp;

    auto mem = std::to_array<uint8_t>({
      mmu.Read8(pc),
      mmu.Read8(pc + 1),
      mmu.Read8(pc + 2),
      mmu.Read8(pc + 3),
    });

    logger->info("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
      a, f, b, c, d, e, h, l, sp, pc, mem[0], mem[1], mem[2], mem[3]);
  }

  uint8_t byte_code = ReadNext8();
  Instruction instr = Decoder::Decode(byte_code);

  if (instr.opcode == Opcode::PREFIX) {
    byte_code = ReadNext8();
    instr = Decoder::DecodePrefixed(byte_code);
  }

  std::visit(overloaded{
     [&](Operands_Imm8 &operands) { operands.imm = ReadNext8(); },
     [&](Operands_Imm16 &operands) { operands.imm = ReadNext16(); },
     [&](Operands_Offset &operands) { operands.offset = static_cast<int8_t>(ReadNext8()); },
     [&](Operands_Reg8_Imm8 &operands) { operands.imm = ReadNext8(); },
     [&](Operands_Reg16_Imm16 &operands) { operands.imm =ReadNext16(); },
     [&](Operands_Cond_Imm16 &operands) { operands.imm = ReadNext16(); },
     [&](Operands_Cond_Offset &operands) { operands.offset = static_cast<int8_t>(ReadNext8()); },
     [&](Operands_SP_Imm16 &operands) { operands.imm = ReadNext16(); },
     [&](Operands_SP_Offset &operands) { operands.offset = static_cast<int8_t>(ReadNext8()); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { operands.addr = ReadNext16(); },
     [&](Operands_Imm16_Ptr_SP &operands) { operands.addr = ReadNext16(); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { operands.addr = ReadNext8(); },
     [&](Operands_Reg8_Imm8_Ptr &operands) { operands.addr = ReadNext8(); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { operands.addr = ReadNext16(); },
     [&](Operands_Reg16_SP_Offset &operands) { operands.offset = static_cast<int8_t>(ReadNext8()); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { operands.imm = ReadNext8(); },
     [&](auto &operands) { /* pass */ }
  }, instr.operands);

  auto cycles = instr.cycles;

  switch (instr.opcode) {
  case Opcode::INVALID:
  case Opcode::NOP: break;
  case Opcode::LD: execute_ld(*this, mmu, instr); break;
  case Opcode::INC: execute_inc(*this, mmu, instr); break;
  case Opcode::DEC: execute_dec(*this, mmu, instr); break;
  case Opcode::ADD: execute_add(*this, mmu, instr); break;
  case Opcode::ADC: execute_adc(*this, mmu, instr); break;
  case Opcode::SUB: execute_sub(*this, mmu, instr); break;
  case Opcode::SBC: execute_sbc(*this, mmu, instr); break;
  case Opcode::AND: execute_and(*this, mmu, instr); break;
  case Opcode::XOR: execute_xor(*this, mmu, instr); break;
  case Opcode::OR: execute_or(*this, mmu, instr); break;
  case Opcode::CP: execute_cp(*this, mmu, instr); break;
  case Opcode::RLCA: execute_rlca(*this, mmu, instr); break;
  case Opcode::RRCA: execute_rrca(*this, mmu, instr); break;
  case Opcode::RLA: execute_rla(*this, mmu, instr); break;
  case Opcode::RRA: execute_rra(*this, mmu, instr); break;
  case Opcode::DAA: execute_daa(*this, mmu, instr); break;
  case Opcode::CPL: execute_cpl(*this, mmu, instr); break;
  case Opcode::SCF: execute_scf(*this, mmu,instr); break;
  case Opcode::CCF: execute_ccf(*this, mmu, instr); break;
  case Opcode::JR: {
    if (!execute_jr(*this, mmu, instr)) {
      cycles = instr.cycles_cond;
    }
    break;
  }
  case Opcode::HALT: execute_halt(*this, mmu, instr); break;
  case Opcode::LDH: execute_ldh(*this, mmu, instr); break;
  case Opcode::POP: execute_pop(*this, mmu, instr); break;
  case Opcode::PUSH: execute_push(*this, mmu, instr); break;
  case Opcode::RST: execute_rst(*this, mmu, instr); break;
  case Opcode::CALL:
    if (!execute_call(*this, mmu, instr)) {
      cycles = instr.cycles_cond;
    }
    break;
  case Opcode::JP:
    if (!execute_jp(*this, mmu, instr)) {
      cycles = instr.cycles_cond;
    }
    break;
  case Opcode::RET:
    if (!execute_ret(*this, mmu, instr)) {
      cycles = instr.cycles_cond;
    }
    break;
  case Opcode::RETI: execute_reti(*this, mmu, instr); break;
  case Opcode::RLC: execute_rlc(*this, mmu, instr); break;
  case Opcode::RRC: execute_rrc(*this, mmu, instr); break;
  case Opcode::RL: execute_rl(*this, mmu, instr); break;
  case Opcode::RR: execute_rr(*this, mmu, instr); break;
  case Opcode::SLA: execute_sla(*this, mmu, instr); break;
  case Opcode::SRA: execute_sra(*this, mmu, instr); break;
  case Opcode::SWAP: execute_swap(*this, mmu, instr); break;
  case Opcode::SRL: execute_srl(*this, mmu, instr); break;
  case Opcode::BIT: execute_bit(*this, mmu, instr); break;
  case Opcode::RES: execute_res(*this, mmu, instr); break;
  case Opcode::SET: execute_set(*this, mmu, instr); break;
  case Opcode::DI: execute_di(*this, mmu, instr); break;
  case Opcode::EI: execute_ei(*this, mmu, instr); break;
  case Opcode::STOP: execute_stop(*this, mmu, instr); break;
  case Opcode::PREFIX: break;
  }

  return cycles;
}

uint8_t Cpu::ExecuteInterrupts() {
  if (!state.ime && !state.halt) {
    return 0;
  }

  for (int i = 0; i < std::to_underlying(Interrupt::Count); i++) {
    Interrupt interrupt { i };
    if (interrupts.IsInterruptRequested(interrupt)) {
      state.halt = false;
      if (!state.ime) {
        return 0;
      }

      state.ime = false;
      interrupts.ClearInterrupt(interrupt);

      Tick();
      Tick();

      auto handler_addr = interrupt_handler(interrupt);
      instr_call_imm16(*this, handler_addr);

      return 20;
    }
  }

  return 0;
}

uint8_t Cpu::ReadNext8() {
  return Read8(regs.pc++);
}

uint16_t Cpu::ReadNext16() {
  auto lo = Read8(regs.pc++);
  auto hi = Read8(regs.pc++);
  auto result = lo | (hi << 8);
  return result;
}

void Cpu::Reset() {
  regs.reset();
  state.Reset();
}

Cpu::Cpu(Mmu &mmu_, InterruptDevice &interrupts_):mmu{mmu_}, interrupts{interrupts_} {}

uint8_t Cpu::Read8(uint16_t addr) {
  auto result = mmu.Read8(addr);
  Tick();
  return result;
}

void Cpu::Write8(uint16_t addr, uint8_t val) {
  mmu.Write8(addr, val);
  Tick();
}


uint16_t Cpu::Read16(uint16_t addr) {
  uint8_t lo = mmu.Read8(addr);
  Tick();
  uint8_t hi = mmu.Read8(addr + 1);
  Tick();
  return lo | (hi << 8);
}

void Cpu::Write16(uint16_t addr, uint16_t word) {
  mmu.Write8(addr, word & 0xff);
  Tick();
  mmu.Write8(addr + 1, word >> 8);
  Tick();
}


void Cpu::Push16(uint16_t word) {
  mmu.Write8(--regs.sp, word >> 8);
  Tick();
  mmu.Write8(--regs.sp, word & 0xff);
  Tick();
}

uint16_t Cpu::Pop16() {
  uint8_t lo = mmu.Read8(regs.sp++);
  Tick();
  uint8_t hi = mmu.Read8(regs.sp++);
  Tick();
  return lo | (hi << 8);
}

void Cpu::AddSyncedDevice(SyncedDevice *device) {
  synced_devices.push_back(device);
}

void Cpu::Tick() {
  for (auto &device : synced_devices) {
    device->OnTick();
  }
  tick_counter += 4;
}

uint64_t Cpu::Ticks() const {
  return tick_counter;
}
