#include <cassert>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>
#include <tracy/Tracy.hpp>

#include "cpu.hpp"
#include "opcodes.hpp"
#include "overloaded.hpp"
#include "instructions.hpp"


inline u16 interrupt_handler(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank: return 0x40;
    case Interrupt::Stat: return 0x48;
    case Interrupt::Timer: return 0x50;
    case Interrupt::Serial: return 0x58;
    case Interrupt::Joypad: return 0x60;
    default: std::unreachable();
  }
}

inline bool check_cond(Cpu& cpu, Cond cond) {
  switch (cond) {
  case Cond::NZ: return !cpu.GetRegisters().Get(Flag::Z);
  case Cond::Z: return cpu.GetRegisters().Get(Flag::Z);
  case Cond::NC: return !cpu.GetRegisters().Get(Flag::C);
  case Cond::C: return cpu.GetRegisters().Get(Flag::C);
  default: std::unreachable();
  }
}

inline void instr_load_reg8_reg8(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.GetRegisters().At(r1) = cpu.GetRegisters().Get(r2);
}

inline void instr_load_reg8_imm8(Cpu& cpu, Reg8 r1, u8 imm) {
  cpu.GetRegisters().At(r1) = imm;
}

inline void instr_load_reg8_reg16_ptr(Cpu& cpu, Reg8 r1, Reg16 r2) {
  cpu.GetRegisters().At(r1) = cpu.Read8(cpu.GetRegisters().Get(r2));
}

inline void instr_load_reg16_ptr_reg8(Cpu& cpu, Reg16 r1, Reg8 r2) {
  cpu.Write8(cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2));
}

inline void instr_load_reg16_ptr_imm8(Cpu& cpu, Reg16 r1, u8 val) {
  cpu.Write8(cpu.GetRegisters().Get(r1), val);
}

inline void instr_load_reg8_imm16_ptr(Cpu& cpu, Reg8 r1, u16 val) {
  cpu.GetRegisters().At(r1) = cpu.Read8(val);
}

inline void instr_load_imm16_ptr_reg8(Cpu& cpu, u16 val, Reg8 r1) {
  cpu.Write8(val, cpu.GetRegisters().Get(r1));
}

inline void instr_load_hi_reg8_reg8_ptr(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.GetRegisters().At(r1) = cpu.Read8(0xff00 | cpu.GetRegisters().Get(r2));
}

inline void instr_load_hi_reg8_ptr_reg8(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.Write8(0xff00 | cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2));
}

inline void instr_load_hi_reg8_imm8_ptr(Cpu& cpu, Reg8 r1, u8 val) {
  cpu.GetRegisters().At(r1) = cpu.Read8(0xff00 | val);
}

inline void instr_load_hi_imm8_ptr_reg8(Cpu& cpu, u8 val, Reg8 r1) {
  cpu.Write8(0xff00 | val, cpu.GetRegisters().Get(r1));
}

inline void instr_load_reg8_reg16_ptr_dec(Cpu& cpu, Reg8 r1, Reg16 r2) {
  auto addr = cpu.GetRegisters().Get(r2);
  cpu.GetRegisters().At(r1) = cpu.Read8(addr);
  cpu.GetRegisters().Set(r2, addr - 1);
}

inline void instr_load_reg16_ptr_dec_reg8(Cpu& cpu, Reg16 r1, Reg8 r2) {
  auto addr = cpu.GetRegisters().Get(r1);
  cpu.Write8(addr, cpu.GetRegisters().Get(r2));
  cpu.GetRegisters().Set(r1, addr - 1);
}

inline void instr_load_reg8_reg16_ptr_inc(Cpu& cpu, Reg8 r1, Reg16 r2) {
  auto addr = cpu.GetRegisters().Get(r2);
  cpu.GetRegisters().At(r1) = cpu.Read8(addr);
  cpu.GetRegisters().Set(r2, addr + 1);
}

inline void instr_load_reg16_ptr_inc_reg8(Cpu& cpu, Reg16 r1, Reg8 r2) {
  auto addr = cpu.GetRegisters().Get(r1);
  cpu.Write8(addr, cpu.GetRegisters().Get(r2));
  cpu.GetRegisters().Set(r1, addr + 1);
}

inline void instr_load_reg16_reg16(Cpu& cpu, Reg16 r1, Reg16 r2) {
  cpu.GetRegisters().Set(r1, cpu.GetRegisters().Get(r2));
  cpu.Tick();
}

inline void instr_load_reg16_imm16(Cpu& cpu, Reg16 r1, u16 val) {
  cpu.GetRegisters().Set(r1, val);
}

inline void instr_load_imm16_ptr_sp(Cpu& cpu, u16 val) {
  cpu.Write16(val, cpu.GetRegisters().sp);
}

inline void instr_load_sp_reg16(Cpu& cpu, Reg16 r1) {
  cpu.GetRegisters().sp = cpu.GetRegisters().Get(r1);
  cpu.Tick();
}

inline void instr_load_sp_imm16(Cpu& cpu, u16 imm) {
  cpu.GetRegisters().sp = imm;
}

inline void instr_load_reg16_sp_offset(Cpu& cpu, Reg16 r1, i8 e) {
  u32 sp = cpu.GetRegisters().sp;
  u32 result = sp + e;

  cpu.GetRegisters().Set(r1, result);
  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  cpu.GetRegisters().Set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);
  cpu.Tick();
}

inline void instr_push_reg16(Cpu& cpu, Reg16 r1) {
  u16 val = cpu.GetRegisters().Get(r1);
  cpu.Push16(val);
}

inline void instr_pop_reg16(Cpu& cpu, Reg16 r1) {
  auto val = cpu.Pop16();
  cpu.GetRegisters().Set(r1, val);
}

inline u8 instr_add8(Cpu& cpu, u8 a, u8 b, u8 c) {
  u8 result_half = (a & 0xf) + (b& 0xf) + c;
  u16 result = a + b + c;

  cpu.GetRegisters().Set(Flag::Z, (result & 0xff) == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (result_half >> 4) & 0x1);
  cpu.GetRegisters().Set(Flag::C, (result >> 8) & 0x1);

  return (u8)result;
}

inline u16 instr_add16(Cpu& cpu, u16 a, u16 b) {
  u16 result_half = (a & 0xfff) + (b & 0xfff);
  u32 result = a + b;

  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (result_half >> 12) & 0x1);
  cpu.GetRegisters().Set(Flag::C, (result >> 16) & 0x1);

  return result;
}

inline void instr_add_reg8(Cpu& cpu, Reg8 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.GetRegisters().Get(r), 0);
}

inline void instr_add_reg16_ptr(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.Read8(cpu.GetRegisters().Get(r)), 0);
}

inline void instr_add_imm8(Cpu& cpu, u8 imm) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), imm, 0);
}

inline void instr_add_reg8_reg8(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.GetRegisters().At(r1) = instr_add8(cpu, cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2), 0);
}

inline void instr_add_reg8_imm8(Cpu& cpu, Reg8 r, u8 imm) {
  cpu.GetRegisters().At(r) = instr_add8(cpu, cpu.GetRegisters().Get(r), imm, 0);
}

inline void instr_add_reg8_reg16_ptr(Cpu& cpu, Reg8 r1, Reg16 r2) {
  cpu.GetRegisters().At(r1) = instr_add8(cpu, cpu.GetRegisters().Get(r1), cpu.Read8(cpu.GetRegisters().Get(r2)), 0);
}

inline void instr_add_carry_reg8(Cpu& cpu, Reg8 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.GetRegisters().Get(r), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_carry_reg8_reg8(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.GetRegisters().At(r1) = instr_add8(cpu, cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_carry_reg8_imm8(Cpu& cpu, Reg8 r, u8 imm) {
  cpu.GetRegisters().At(r) = instr_add8(cpu, cpu.GetRegisters().Get(r), imm, cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_carry_reg8_reg16_ptr(Cpu& cpu, Reg8 r1, Reg16 r2) {
  cpu.GetRegisters().At(r1) = instr_add8(cpu, cpu.GetRegisters().Get(r1), cpu.Read8(cpu.GetRegisters().Get(r2)), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_carry_reg16_ptr(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.Read8(cpu.GetRegisters().Get(r)), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_carry_imm8(Cpu& cpu, u8 imm) {
  cpu.GetRegisters().At(Reg8::A) = instr_add8(cpu, cpu.GetRegisters().Get(Reg8::A), imm, cpu.GetRegisters().Get(Flag::C));
}

inline void instr_add_reg16_sp(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().Set(r, instr_add16(cpu, cpu.GetRegisters().Get(r), cpu.GetRegisters().sp));
  cpu.Tick();
}

inline void instr_add_reg16_reg16(Cpu& cpu, Reg16 r1, Reg16 r2) {
  cpu.GetRegisters().Set(r1, instr_add16(cpu, cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2)));
  cpu.Tick();
}

inline void instr_add_sp_offset(Cpu& cpu, i8 e) {
  u32 sp = cpu.GetRegisters().sp;
  u32 result = sp + e;

  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  cpu.GetRegisters().Set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);

  cpu.GetRegisters().sp = result;
  cpu.Tick();
  cpu.Tick();
}

inline u8 instr_sub8(Cpu& cpu, u8 a, u8 b, u8 c) {
  auto half_result = static_cast<i16>(a & 0xf) - (b & 0xf) - c;
  auto result = static_cast<i16>(a) - b - c;

  cpu.GetRegisters().Set(Flag::Z, (u8)result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 1);
  cpu.GetRegisters().Set(Flag::H, half_result < 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::C, result < 0 ? 1 : 0);

  return result;
}

inline void instr_sub_reg8(Cpu& cpu, Reg8 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.GetRegisters().Get(r), 0);
}

inline void instr_sub_reg16_ptr(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.Read8(cpu.GetRegisters().Get(r)), 0);
}

inline void instr_sub_imm8(Cpu& cpu, u8 imm) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), imm, 0);
}

inline void instr_sub_carry_reg8(Cpu& cpu, Reg8 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.GetRegisters().Get(r), 0);
}

inline void instr_sub_carry_reg16_ptr(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.Read8(cpu.GetRegisters().Get(r)), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_sub_carry_imm8(Cpu& cpu, u8 imm) {
  cpu.GetRegisters().At(Reg8::A) = instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), imm, cpu.GetRegisters().Get(Flag::C));
}

inline void instr_sub_carry_reg8_reg8(Cpu& cpu, Reg8 r1, Reg8 r2) {
  cpu.GetRegisters().At(r1) = instr_sub8(cpu, cpu.GetRegisters().Get(r1), cpu.GetRegisters().Get(r2), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_sub_carry_reg8_imm8(Cpu& cpu, Reg8 r, u8 imm) {
  cpu.GetRegisters().At(r) = instr_sub8(cpu, cpu.GetRegisters().Get(r), imm, cpu.GetRegisters().Get(Flag::C));
}

inline void instr_sub_carry_reg8_reg16_ptr(Cpu& cpu, Reg8 r1, Reg16 r2) {
  cpu.GetRegisters().At(r1) = instr_sub8(cpu, cpu.GetRegisters().Get(r1), cpu.Read8(cpu.GetRegisters().Get(r2)), cpu.GetRegisters().Get(Flag::C));
}

inline void instr_cmp_reg8(Cpu& cpu, Reg8 r) {
  instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.GetRegisters().Get(r), 0);
}

inline void instr_cmp_reg16_ptr(Cpu& cpu, Reg16 r) {
  instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), cpu.Read8(cpu.GetRegisters().Get(r)), 0);
}

inline void instr_cmp_imm8(Cpu& cpu, u8 imm) {
  instr_sub8(cpu, cpu.GetRegisters().Get(Reg8::A), imm, 0);
}

inline void instr_inc_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 result = val + 1;
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 result = val + 1;
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_sp(Cpu& cpu) {
  cpu.GetRegisters().sp += 1;
  cpu.Tick();
}

inline void instr_inc_reg16(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().Set(r, cpu.GetRegisters().Get(r) + 1);
  cpu.Tick();
}

inline void instr_dec_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 result = val - 1;
  cpu.GetRegisters().At(r) = result;
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 1);
  cpu.GetRegisters().Set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 result = val - 1;
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 1);
  cpu.GetRegisters().Set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_sp(Cpu& cpu) {
  cpu.GetRegisters().sp -= 1;
  cpu.Tick();
}

inline void instr_dec_reg16(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().Set(r, cpu.GetRegisters().Get(r) - 1);
  cpu.Tick();
}

inline void instr_and_reg8(Cpu& cpu, Reg8 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) &= cpu.GetRegisters().Get(r);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 1);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_and_reg16_ptr(Cpu& cpu, Reg16 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) &= cpu.Read8(cpu.GetRegisters().Get(r));
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 1);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_and_imm8(Cpu& cpu, u8 imm) {
  auto result = cpu.GetRegisters().At(Reg8::A) &= imm;
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 1);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_or_reg8(Cpu& cpu, Reg8 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) |= cpu.GetRegisters().Get(r);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_or_reg16_ptr(Cpu& cpu, Reg16 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) |= cpu.Read8(cpu.GetRegisters().Get(r));
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_or_imm8(Cpu& cpu, u8 imm) {
  auto result = cpu.GetRegisters().At(Reg8::A) |= imm;
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_xor_reg8(Cpu& cpu, Reg8 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) ^= cpu.GetRegisters().Get(r);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_xor_reg16_ptr(Cpu& cpu, Reg16 r) {
  auto result = cpu.GetRegisters().At(Reg8::A) ^= cpu.Read8(cpu.GetRegisters().Get(r));
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_xor_imm8(Cpu& cpu, u8 imm) {
  auto result = cpu.GetRegisters().At(Reg8::A) ^= imm;
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_ccf(Cpu& cpu) {
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, cpu.GetRegisters().Get(Flag::C) ? 0 : 1);
}

inline void instr_scf(Cpu& cpu) {
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 1);
}

inline void instr_daa(Cpu& cpu) {
  auto a = cpu.GetRegisters().Get(Reg8::A);
  auto n = cpu.GetRegisters().Get(Flag::N);
  auto h = cpu.GetRegisters().Get(Flag::H);
  auto c = cpu.GetRegisters().Get(Flag::C);

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
      cpu.GetRegisters().Set(Flag::C, 1);
    }
    if (h || (a & 0x0f) > 0x09) {
      a += 0x6;
    }
  }

  cpu.GetRegisters().Set(Reg8::A, a);
  cpu.GetRegisters().Set(Flag::Z, a == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::H, 0);
}

inline void instr_cpl(Cpu& cpu) {
  cpu.GetRegisters().Set(Reg8::A, ~cpu.GetRegisters().Get(Reg8::A));
  cpu.GetRegisters().Set(Flag::N, 1);
  cpu.GetRegisters().Set(Flag::H, 1);
}

inline void instr_rlca(Cpu& cpu) {
  u8 val = cpu.GetRegisters().Get(Reg8::A);
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | carry;
  cpu.GetRegisters().Set(Reg8::A, result);
  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rrca(Cpu& cpu) {
  u8 val = cpu.GetRegisters().Get(Reg8::A);
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (carry << 7);
  cpu.GetRegisters().Set(Reg8::A, result);
  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rla(Cpu& cpu) {
  u8 val = cpu.GetRegisters().Get(Reg8::A);
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | cpu.GetRegisters().Get(Flag::C);
  cpu.GetRegisters().Set(Reg8::A, result);
  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C,  carry);
}

inline void instr_rra(Cpu& cpu) {
  u8 val = cpu.GetRegisters().Get(Reg8::A);
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (cpu.GetRegisters().Get(Flag::C) << 7);
  cpu.GetRegisters().Set(Reg8::A, result);
  cpu.GetRegisters().Set(Flag::Z, 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rlc_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | carry;
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rlc_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | carry;
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rrc_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (carry << 7);
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rrc_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (carry << 7);
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rl_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | cpu.GetRegisters().Get(Flag::C);
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C,  carry);
}

inline void instr_rl_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = (val & 0x80) >> 7;
  u8 result = (val << 1) | cpu.GetRegisters().Get(Flag::C);
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C,  carry);
}

inline void instr_rr_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (cpu.GetRegisters().Get(Flag::C) << 7);
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_rr_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (cpu.GetRegisters().Get(Flag::C) << 7);
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_sla_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = (val & 0x80) >> 7;
  u8 result = val << 1;
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_sla_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = (val & 0x80) >> 7;
  u8 result = val << 1;
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_srl_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = val & 0x1;
  u8 result = val >> 1;
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_srl_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = val & 0x1;
  u8 result = val >> 1;
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_sra_reg8(Cpu& cpu, Reg8 r) {
  u8 val = cpu.GetRegisters().Get(r);
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (val & 0x80);
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_sra_reg16_ptr(Cpu& cpu, Reg16 r) {
  u8 val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 carry = val & 0x1;
  u8 result = (val >> 1) | (val & 0x80);
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, carry);
}

inline void instr_swap_reg8(Cpu& cpu, Reg8 r) {
  auto val = cpu.GetRegisters().Get(r);
  u8 upper = (val >> 4) & 0xf;
  u8 lower = val & 0xf;
  u8 result = upper | (lower << 4);
  cpu.GetRegisters().Set(r, result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_swap_reg16_ptr(Cpu& cpu, Reg16 r) {
  auto val = cpu.Read8(cpu.GetRegisters().Get(r));
  u8 upper = (val >> 4) & 0xf;
  u8 lower = val & 0xf;
  u8 result = upper | (lower << 4);
  cpu.Write8(cpu.GetRegisters().Get(r), result);
  cpu.GetRegisters().Set(Flag::Z, result == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 0);
  cpu.GetRegisters().Set(Flag::C, 0);
}

inline void instr_bit_imm8_reg8(Cpu& cpu, u8 imm, Reg8 r) {
  auto bit = (cpu.GetRegisters().Get(r) >> imm) & 0x1;
  cpu.GetRegisters().Set(Flag::Z, bit == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 1);
}

inline void instr_bit_imm8_reg16_ptr(Cpu& cpu, u8 imm, Reg16 r) {
  auto bit = (cpu.Read8(cpu.GetRegisters().Get(r)) >> imm) & 0x1;
  cpu.GetRegisters().Set(Flag::Z, bit == 0 ? 1 : 0);
  cpu.GetRegisters().Set(Flag::N, 0);
  cpu.GetRegisters().Set(Flag::H, 1);
}

inline void instr_res_imm8_reg8(Cpu& cpu, u8 imm, Reg8 r) {
  u8 mask = cpu.GetRegisters().Get(r) & (1 << imm);
  cpu.GetRegisters().At(r) ^= mask;
}

inline void instr_res_imm8_reg16_ptr(Cpu& cpu, u8 imm, Reg16 r) {
  u16 addr = cpu.GetRegisters().Get(r);
  u8 val = cpu.Read8(addr);
  u8 mask = val & (1 << imm);
  u8 result = val ^ mask;
  cpu.Write8(addr, result);
}

inline void instr_set_imm8_reg8(Cpu& cpu, u8 imm, Reg8 r) {
  u8 mask = 1 << imm;
  cpu.GetRegisters().At(r) |= mask;
}

inline void instr_set_imm8_reg16_ptr(Cpu& cpu, u8 imm, Reg16 r) {
  u8 mask = 1 << imm;
  u16 addr = cpu.GetRegisters().Get(r);
  u8 result = cpu.Read8(addr) | mask;
  cpu.Write8(addr, result);
}

inline bool instr_jump_imm16(Cpu& cpu, u16 imm) {
  cpu.GetRegisters().pc = imm;
  cpu.Tick();
  return true;
}

inline bool instr_jump_reg16(Cpu& cpu, Reg16 r) {
  cpu.GetRegisters().pc = cpu.GetRegisters().Get(r);
  return true;
}

inline bool instr_jump_cond_imm16(Cpu& cpu, Cond cond, u16 nn) {
  if (check_cond(cpu, cond)) {
    cpu.GetRegisters().pc = nn;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_jump_rel(Cpu& cpu, i8 e) {
  cpu.GetRegisters().pc += e;
  cpu.Tick();
  return true;
}

inline bool instr_jump_rel_cond(Cpu& cpu, Cond cond, i8 e) {
  if (check_cond(cpu, cond)) {
    cpu.GetRegisters().pc += e;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_call_imm16(Cpu& cpu, u16 nn) {
  cpu.Push16(cpu.GetRegisters().pc);
  cpu.GetRegisters().pc = nn;
  cpu.Tick();
  return true;
}

inline bool instr_call_cond_imm16(Cpu& cpu, Cond cond, u16 nn) {
  if (check_cond(cpu, cond)) {
    cpu.Push16(cpu.GetRegisters().pc);
    cpu.GetRegisters().pc = nn;
    cpu.Tick();
    return true;
  }
  return false;
}

inline bool instr_ret(Cpu& cpu) {
  cpu.GetRegisters().pc = cpu.Pop16();
  cpu.Tick();
  return true;
}

inline bool instr_ret_cond(Cpu& cpu, Cond cond) {
  auto jump = check_cond(cpu, cond);
  cpu.Tick();
  if (jump) {
    cpu.GetRegisters().pc = cpu.Pop16();
    cpu.Tick();
    return true;
  }
  return false;
}

inline void instr_reti(Cpu& cpu) {
  cpu.GetRegisters().pc = cpu.Pop16();
  cpu.GetState().ime = true;
  cpu.Tick();
}

inline void instr_restart(Cpu& cpu, u8 n) {
  cpu.Push16(cpu.GetRegisters().pc);
  cpu.GetRegisters().pc = n;
  cpu.Tick();
}

inline void execute_ld(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_SP_Reg16& operands) { instr_load_sp_reg16(cpu, operands.reg); },
     [&](Operands_SP_Imm16& operands) { instr_load_sp_imm16(cpu, operands.imm); },
     [&](Operands_Reg8_Reg8& operands) { instr_load_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_load_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg16_Reg16& operands) { instr_load_reg16_reg16(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Imm16& operands) { instr_load_reg16_imm16(cpu, operands.reg, operands.imm); },
     [&](Operands_Imm8_Ptr_Reg8& operands) { instr_load_hi_imm8_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg16_Ptr& operands) { instr_load_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Inc& operands) { instr_load_reg8_reg16_ptr_inc(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Dec& operands) { instr_load_reg8_reg16_ptr_dec(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm16_Ptr& operands) { instr_load_reg8_imm16_ptr(cpu, operands.reg, operands.addr); },
     [&](Operands_Reg16_SP_Offset& operands) { instr_load_reg16_sp_offset(cpu, operands.reg, operands.offset); },
     [&](Operands_Reg16_Ptr_Reg8& operands) { instr_load_reg16_ptr_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Inc_Reg8& operands) { instr_load_reg16_ptr_inc_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Imm8& operands) { instr_load_reg16_ptr_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg16_Ptr_Dec_Reg8& operands) { instr_load_reg16_ptr_dec_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Imm16_Ptr_Reg8& operands) { instr_load_imm16_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Imm16_Ptr_SP& operands) { instr_load_imm16_ptr_sp(cpu, operands.addr); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

inline void execute_inc(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_inc_reg8(cpu, operands.reg); },
     [&](Operands_Reg16& operands) { instr_inc_reg16(cpu, operands.reg); },
     [&](Operands_Reg16_Ptr& operands) { instr_inc_reg16_ptr(cpu, operands.reg); },
     [&](Operands_SP& operands) { instr_inc_sp(cpu); },
     [&](auto&) { std::unreachable(); },
  }, instr.operands);
}

void execute_dec(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_dec_reg8(cpu, operands.reg); },
     [&](Operands_Reg16& operands) { instr_dec_reg16(cpu, operands.reg); },
     [&](Operands_Reg16_Ptr& operands) { instr_dec_reg16_ptr(cpu, operands.reg); },
     [&](Operands_SP& operands) { instr_dec_sp(cpu); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_add(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_add_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_add_imm8(cpu, operands.imm); },
     [&](Operands_Reg8_Reg8& operands) { instr_add_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_add_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr& operands) { instr_add_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Reg16& operands) { instr_add_reg16_reg16(cpu, operands.reg1, operands.reg2); },
     [&](Operands_SP_Offset& operands) { instr_add_sp_offset(cpu, operands.offset); },
     [&](Operands_Reg16_Ptr& operands) { instr_add_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg16_SP& operands) { instr_add_reg16_sp(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_adc(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_add_carry_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_add_carry_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_add_carry_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg8_Reg8& operands) { instr_add_carry_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_add_carry_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr& operands) { instr_add_carry_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sub(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_sub_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_sub_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_sub_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sbc(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_sub_carry_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_sub_carry_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_sub_carry_reg16_ptr(cpu, operands.reg); },
     [&](Operands_Reg8_Reg8& operands) { instr_sub_carry_reg8_reg8(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_sub_carry_reg8_imm8(cpu, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr& operands) { instr_sub_carry_reg8_reg16_ptr(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_and(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_and_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_and_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_and_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_xor(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_xor_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_xor_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_xor_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_or(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_or_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_or_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_or_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_cp(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8& operands) { instr_cmp_reg8(cpu, operands.reg); },
     [&](Operands_Imm8& operands) { instr_cmp_imm8(cpu, operands.imm); },
     [&](Operands_Reg16_Ptr& operands) { instr_cmp_reg16_ptr(cpu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_rlca(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_rlca(cpu);
}

void execute_rrca(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_rrca(cpu);
}

void execute_rla(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_rla(cpu);
}

void execute_rra(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_rra(cpu);
}

void execute_daa(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_daa(cpu);
}

void execute_cpl(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_cpl(cpu);
}

void execute_scf(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_scf(cpu);
}

void execute_ccf(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_ccf(cpu);
}

bool execute_jr(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  return std::visit(overloaded{
     [&](Operands_Offset& operands) { return instr_jump_rel(cpu, operands.offset); },
     [&](Operands_Cond_Offset& operands) { return instr_jump_rel_cond(cpu, operands.cond, operands.offset); },
     [&](auto &) { std::unreachable(); return false; },
  }, instr.operands);
}

void execute_halt(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  cpu.GetState().halt = true;
}

void execute_ldh(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  std::visit(overloaded{
     [&](Operands_Reg8_Imm8_Ptr& operands) { instr_load_hi_reg8_imm8_ptr(cpu, operands.reg, operands.addr); },
     [&](Operands_Imm8_Ptr_Reg8& operands) { instr_load_hi_imm8_ptr_reg8(cpu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg8_Ptr& operands) { instr_load_hi_reg8_reg8_ptr(cpu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Ptr_Reg8& operands) { instr_load_hi_reg8_ptr_reg8(cpu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_pop(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_pop_reg16(cpu, operands.reg);
}

void execute_push(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  cpu.Tick();
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_push_reg16(cpu, operands.reg);
}

void execute_rst(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  auto operands = std::get<Operands_Imm8_Literal>(instr.operands);
  instr_restart(cpu, operands.imm);
}

bool execute_call(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Imm16>(&instr.operands)) {
    return instr_call_imm16(cpu, ops->imm);
  } else if (const auto* ops = std::get_if<Operands_Cond_Imm16>(&instr.operands)) {
    return instr_call_cond_imm16(cpu, ops->cond, ops->imm);
  }
  std::unreachable();
}

bool execute_jp(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  return std::visit(overloaded{
    [&](Operands_Reg16& operands) { return instr_jump_reg16(cpu, operands.reg); },
    [&](Operands_Imm16& operands) { return instr_jump_imm16(cpu, operands.imm); },
    [&](Operands_Cond_Imm16& operands) { return instr_jump_cond_imm16(cpu, operands.cond, operands.imm); },
    [&](auto &) { std::unreachable(); return false; },
  }, instr.operands);
}

bool execute_ret(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (std::get_if<Operands_None>(&instr.operands)) {
    return instr_ret(cpu);
  } else if (const auto* ops = std::get_if<Operands_Cond>(&instr.operands)) {
    return instr_ret_cond(cpu, ops->cond);
  }
  std::unreachable();
}

void execute_reti(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  instr_reti(cpu);
}

void execute_rlc(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rlc_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rlc_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rrc(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rrc_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rrc_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rl(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rl_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rl_reg16_ptr(cpu, ops->reg);
  }
}

void execute_rr(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rr_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rr_reg16_ptr(cpu, ops->reg);
  }
}

void execute_sla(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sla_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sla_reg16_ptr(cpu, ops->reg);
  }
}

void execute_sra(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sra_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sra_reg16_ptr(cpu, ops->reg);
  }
}

void execute_srl(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_srl_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_srl_reg16_ptr(cpu, ops->reg);
  }
}

void execute_swap(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_swap_reg8(cpu,  ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_swap_reg16_ptr(cpu, ops->reg);
  }
}

void execute_bit(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_bit_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_bit_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_res(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_res_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_res_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_set(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_set_imm8_reg8(cpu, ops->imm, ops->reg);
  } else if (const auto* ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_set_imm8_reg16_ptr(cpu, ops->imm, ops->reg);
  }
}

void execute_di(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  cpu.GetState().ime = false;
}

void execute_ei(Cpu& cpu, Mmu& mmu, Instruction& instr) {
  if (!cpu.GetState().ime) {
    cpu.GetState().ime_trigger = true;
  }
}

void Cpu::Init(CpuConfig cfg) {
  mmu_ = cfg.mmu;
  interrupts_ = cfg.interrupts;
  test_ = cfg.test;
}

u8 Cpu::Execute() {
  ZoneScoped;

  if (state_.stop) {
    return 4;
  }

  tick_counter = 0;

  auto interrupt_cycles = ExecuteInterrupts();
  if (interrupt_cycles) {
    return interrupt_cycles;
  }

  if (state_.halt) {
    Tick();
    return 4;
  }

  auto logger = spdlog::get("doctor_logger");
  if (logger) {
    auto a = regs_.Get(Reg8::A);
    auto f = regs_.Get(Reg8::F);
    auto b = regs_.Get(Reg8::B);
    auto c = regs_.Get(Reg8::C);
    auto d = regs_.Get(Reg8::D);
    auto e = regs_.Get(Reg8::E);
    auto h = regs_.Get(Reg8::H);
    auto l = regs_.Get(Reg8::L);
    auto pc = regs_.pc;
    auto sp = regs_.sp;

    auto mem = std::to_array<u8>({
      mmu_->Read8(pc),
      mmu_->Read8(pc + 1),
      mmu_->Read8(pc + 2),
      mmu_->Read8(pc + 3),
    });

    logger->info("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
      a, f, b, c, d, e, h, l, sp, pc, mem[0], mem[1], mem[2], mem[3]);
  }

  u8 byte_code = ReadNext8();
  Instruction instr = Decoder::Decode(byte_code);

  if (instr.opcode == Opcode::PREFIX) {
    byte_code = ReadNext8();
    instr = Decoder::DecodePrefixed(byte_code);
  }

  std::visit(overloaded{
     [&](Operands_Imm8& operands) { operands.imm = ReadNext8(); },
     [&](Operands_Imm16& operands) { operands.imm = ReadNext16(); },
     [&](Operands_Offset& operands) { operands.offset = static_cast<i8>(ReadNext8()); },
     [&](Operands_Reg8_Imm8& operands) { operands.imm = ReadNext8(); },
     [&](Operands_Reg16_Imm16& operands) { operands.imm =ReadNext16(); },
     [&](Operands_Cond_Imm16& operands) { operands.imm = ReadNext16(); },
     [&](Operands_Cond_Offset& operands) { operands.offset = static_cast<i8>(ReadNext8()); },
     [&](Operands_SP_Imm16& operands) { operands.imm = ReadNext16(); },
     [&](Operands_SP_Offset& operands) { operands.offset = static_cast<i8>(ReadNext8()); },
     [&](Operands_Imm16_Ptr_Reg8& operands) { operands.addr = ReadNext16(); },
     [&](Operands_Imm16_Ptr_SP& operands) { operands.addr = ReadNext16(); },
     [&](Operands_Imm8_Ptr_Reg8& operands) { operands.addr = ReadNext8(); },
     [&](Operands_Reg8_Imm8_Ptr& operands) { operands.addr = ReadNext8(); },
     [&](Operands_Reg8_Imm16_Ptr& operands) { operands.addr = ReadNext16(); },
     [&](Operands_Reg16_SP_Offset& operands) { operands.offset = static_cast<i8>(ReadNext8()); },
     [&](Operands_Reg16_Ptr_Imm8& operands) { operands.imm = ReadNext8(); },
     [&](auto& operands) { /* pass */ }
  }, instr.operands);

  auto cycles = instr.cycles;
  auto& mmu = *(this->mmu_);

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
  case Opcode::STOP: ExecuteStop(); break;
  case Opcode::PREFIX: break;
  }

  return cycles;
}

u8 Cpu::ExecuteInterrupts() {
  ZoneScoped;

  bool ime_state = state_.ime;

  if (state_.ime_trigger) {
    state_.ime = true;
    state_.ime_trigger = false;
  }

  if (!ime_state && !state_.halt) {
    return 0;
  }

  for (int i = 0; i < std::to_underlying(Interrupt::Count); i++) {
    Interrupt interrupt { i };
    if (interrupts_->IsInterruptRequested(interrupt)) {
      state_.halt = false;
      if (!ime_state) {
        return 0;
      }

      state_.ime = false;

      Tick();
      Tick();

      Write8(--regs_.sp, regs_.pc >> 8);
      bool is_cancelled = true;
      while (i < std::to_underlying(Interrupt::Count)) {
        interrupt = Interrupt{i};
        if (interrupts_->IsInterruptRequested(interrupt)) {
          is_cancelled = false;
          break;
        }
        i++;
      }

      Write8(--regs_.sp, regs_.pc & 0xff);
      if (!is_cancelled) {
        regs_.pc = interrupt_handler(interrupt);
        interrupts_->ClearInterrupt(interrupt);
      } else {
        regs_.pc = 0;
      }

      Tick();

      state_.ime = false;
      return 20;
    }
  }

  return 0;
}

u8 Cpu::ReadNext8() {
  ZoneScoped;
  return Read8(regs_.pc++);
}

u16 Cpu::ReadNext16() {
  ZoneScoped;
  auto lo = Read8(regs_.pc++);
  auto hi = Read8(regs_.pc++);
  auto result = lo | (hi << 8);
  return result;
}

void Cpu::Reset() {
  hardware_mode_ = HardwareMode::kDmgMode;
  regs_.Reset();
  state_.Reset();
  key0_ = 0;
  key1_ = 0;
}

u8 Cpu::Read8(u16 addr) {
  ZoneScoped;

  u8 result;
  if (test_) {
    result = mmu_->Read8(addr);
  } else {
    switch (addr) {
      case std::to_underlying(IO::KEY0): result = hardware_mode_ == HardwareMode::kDmgMode ? 0xFF : key0_; break;
      case std::to_underlying(IO::KEY1): result = hardware_mode_ == HardwareMode::kDmgMode ? 0xFF : key1_; break;
      default: result = mmu_->Read8(addr); break;
    }
  }
  Tick();
  return result;
}

void Cpu::Write8(u16 addr, u8 val) {
  ZoneScoped;
  if (test_) {
    mmu_->Write8(addr, val);
  } else {
    switch (addr) {
      case std::to_underlying(IO::KEY0): key0_ = val; break;
      case std::to_underlying(IO::KEY1): key1_ = val; break;
      default: mmu_->Write8(addr, val); break;
    }
  }
  Tick();
}


u16 Cpu::Read16(u16 addr) {
  u8 lo = Read8(addr);
  u8 hi = Read8(addr + 1);
  return lo | (hi << 8);
}

void Cpu::Write16(u16 addr, u16 word) {
  Write8(addr, word & 0xff);
  Write8(addr + 1, word >> 8);
}


void Cpu::Push16(u16 word) {
  Write8(--regs_.sp, word >> 8);
  Write8(--regs_.sp, word & 0xff);
}

u16 Cpu::Pop16() {
  u8 lo = Read8(regs_.sp++);
  u8 hi = Read8(regs_.sp++);
  return lo | (hi << 8);
}

void Cpu::AddSyncedDevice(SyncedDevice* device) {
  synced_devices.push_back(device);
}

void Cpu::Tick() {
  ZoneScoped;
  for (auto& device : synced_devices) {
    device->OnTick(state_.double_speed);
  }
  tick_counter += 4;
}

uint64_t Cpu::Ticks() const {
  return tick_counter;
}

Registers& Cpu::GetRegisters() {
  return regs_;
}

const Registers& Cpu::GetRegisters() const {
  return regs_;
}

CpuState& Cpu::GetState() {
  return state_;
}

const CpuState& Cpu::GetState() const {
  return state_;
}


HardwareMode Cpu::GetHardwareMode() const {
  return hardware_mode_;
}

void Cpu::SetHardwareMode(HardwareMode mode) {
  hardware_mode_ = mode;
  mmu_->SetHardwareMode(mode);
}

void Cpu::ExecuteStop() {
  if (hardware_mode_ == HardwareMode::kDmgMode) {
    state_.stop = true;
  } else {
    state_.double_speed = key1_ & 0x1;
    if (state_.double_speed) {
      key1_ = 0x80;
    } else {
      key1_ = 0x00;
    }
  }
  Write8(std::to_underlying(IO::DIV), 0);
}
