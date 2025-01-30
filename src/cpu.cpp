#include <cassert>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "cpu.h"
#include "opcodes.h"
#include "overloaded.h"
#include "instructions.h"

namespace Stack {

inline void push8(Mmu &mmu, uint16_t &sp, uint8_t val) {
  sp -= 1;
  mmu.write8(sp, val);
}

inline void push16(Mmu &mmu, uint16_t &sp, uint16_t val) {
  sp -= 2;
  mmu.write16(sp, val);
}

inline uint8_t pop8(const Mmu &mmu, uint16_t &sp) {
  auto result = mmu.read8(sp);
  sp += 1;
  return result;
}

inline uint16_t pop16(const Mmu &mmu, uint16_t &sp) {
  auto result = mmu.read16(sp);
  sp += 2;
  return result;
}

}

inline uint16_t interrupt_handler(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank: return 0x40;
    case Interrupt::Stat: return 0x48;
    case Interrupt::Timer: return 050;
    case Interrupt::Serial: return 0x58;
    case Interrupt::Joypad: return 0x60;
    default: std::unreachable();
  }
}

inline bool check_cond(Registers &regs, Cond cond) {
  switch (cond) {
  case Cond::NZ: return !regs.get(Flag::Z);
  case Cond::Z: return regs.get(Flag::Z);
  case Cond::NC: return !regs.get(Flag::C);
  case Cond::C: return regs.get(Flag::C);
  default: std::unreachable();
  }
}

inline void instr_load_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  regs.at(r1) = regs.get(r2);
}

inline void instr_load_reg8_imm8(Registers &regs, Reg8 r1, uint8_t imm) {
  regs.at(r1) = imm;
}

inline void instr_load_reg8_reg16_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, Reg16 r2) {
  regs.at(r1) = mmu.read8(regs.get(r2));
}

inline void instr_load_reg16_ptr_reg8(Registers &regs, Mmu& mmu, Reg16 r1, Reg8 r2) {
  mmu.write8(regs.get(r1), regs.get(r2));
}

inline void instr_load_reg16_ptr_imm8(Registers &regs, Mmu& mmu, Reg16 r1, uint8_t val) {
  mmu.write8(regs.get(r1), val);
}

inline void instr_load_reg8_imm16_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, uint16_t val) {
  regs.at(r1) = mmu.read8(val);
}

inline void instr_load_imm16_ptr_reg8(Registers &regs, Mmu& mmu, uint16_t val, Reg8 r1) {
  mmu.write8(val, regs.get(r1));
}

inline void instr_load_hi_reg8_reg8_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, Reg8 r2) {
  regs.at(r1) = mmu.read8(0xff00 | regs.get(r2));
}

inline void instr_load_hi_reg8_ptr_reg8(Registers &regs, Mmu& mmu, Reg8 r1, Reg8 r2) {
  mmu.write8(0xff00 | regs.get(r1), regs.get(r2));
}

inline void instr_load_hi_reg8_imm8_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, uint8_t val) {
  regs.at(r1) = mmu.read8(0xff00 | val);
}

inline void instr_load_hi_imm8_ptr_reg8(Registers &regs, Mmu& mmu, uint8_t val, Reg8 r1) {
  mmu.write8(0xff00 | val, regs.get(r1));
}

inline void instr_load_reg8_reg16_ptr_dec(Registers &regs, const Mmu& mmu, Reg8 r1, Reg16 r2) {
  auto addr = regs.get(r2);
  regs.at(r1) = mmu.read8(addr);
  regs.set(r2, addr - 1);
}

inline void instr_load_reg16_ptr_dec_reg8(Registers &regs, Mmu& mmu, Reg16 r1, Reg8 r2) {
  auto addr = regs.get(r1);
  mmu.write8(addr, regs.get(r2));
  regs.set(r1, addr - 1);
}

inline void instr_load_reg8_reg16_ptr_inc(Registers &regs, const Mmu& mmu, Reg8 r1, Reg16 r2) {
  auto addr = regs.get(r2);
  regs.at(r1) = mmu.read8(addr);
  regs.set(r2, addr + 1);
}

inline void instr_load_reg16_ptr_inc_reg8(Registers &regs, Mmu& mmu, Reg16 r1, Reg8 r2) {
  auto addr = regs.get(r1);
  mmu.write8(addr, regs.get(r2));
  regs.set(r1, addr + 1);
}

inline void instr_load_reg16_reg16(Registers &regs, Reg16 r1, Reg16 r2) {
  regs.set(r1, regs.get(r2));
}

inline void instr_load_reg16_imm16(Registers &regs, Reg16 r1, uint16_t val) {
  regs.set(r1, val);
}

inline void instr_load_imm16_ptr_sp(Registers &regs, Mmu& mmu, uint16_t val) {
  mmu.write16(val, regs.sp);
}

inline void instr_load_sp_reg16(Registers &regs, Reg16 r1) {
  regs.sp = regs.get(r1);
}

inline void instr_load_sp_imm16(Registers &regs, uint16_t imm) {
  regs.sp = imm;
}

inline void instr_load_reg16_sp_offset(Registers &regs, Reg16 r1, int8_t e) {
  uint32_t sp = regs.sp;
  uint32_t result = sp + e;

  regs.set(r1, result);
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  regs.set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);
}

inline void instr_push_reg16(Registers &regs, Mmu& mmu, Reg16 r1) {
  uint16_t val = regs.get(r1);
  Stack::push16(mmu, regs.sp, val);
  spdlog::info("pushed [{}:{:04x}]", magic_enum::enum_name(r1), val);
}

inline void instr_pop_reg16(Registers &regs, Mmu& mmu, Reg16 r1) {
  auto val = Stack::pop16(mmu, regs.sp);
  regs.set(r1, val);
  spdlog::info("popped [{}] <= {:04x}", magic_enum::enum_name(r1), val);
}

inline uint8_t instr_add8(Registers &regs, uint8_t a, uint8_t b, uint8_t c) {
  uint8_t result_half = (a & 0xf) + (b &0xf) + c;
  uint16_t result = a + b + c;

  regs.set(Flag::Z, (result & 0xff) == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result_half >> 4) & 0x1);
  regs.set(Flag::C, (result >> 8) & 0x1);

  return (uint8_t)result;
}

inline uint16_t instr_add16(Registers &regs, uint16_t a, uint16_t b) {
  uint16_t result_half = (a & 0xfff) + (b & 0xfff);
  uint32_t result = a + b;

  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result_half >> 12) & 0x1);
  regs.set(Flag::C, (result >> 16) & 0x1);

  return result;
}

inline void instr_add_reg8(Registers &regs, Reg8 r) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), regs.get(r), 0);
}

inline void instr_add_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), mmu.read8(regs.get(r)), 0);
}

inline void instr_add_imm8(Registers &regs, uint8_t imm) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), imm, 0);
}

inline void instr_add_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  regs.at(r1) = instr_add8(regs, regs.get(r1), regs.get(r2), 0);
}

inline void instr_add_reg8_imm8(Registers &regs, Reg8 r, uint8_t imm) {
  regs.at(r) = instr_add8(regs, regs.get(r), imm, 0);
}

inline void instr_add_reg8_reg16_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, Reg16 r2) {
  regs.at(r1) = instr_add8(regs, regs.get(r1), mmu.read8(regs.get(r2)), 0);
}

inline void instr_add_carry_reg8(Registers &regs, Reg8 r) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), regs.get(r), regs.get(Flag::C));
}

inline void instr_add_carry_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  regs.at(r1) = instr_add8(regs, regs.get(r1), regs.get(r2), regs.get(Flag::C));
}

inline void instr_add_carry_reg8_imm8(Registers &regs, Reg8 r, uint8_t imm) {
  regs.at(r) = instr_add8(regs, regs.get(r), imm, regs.get(Flag::C));
}

inline void instr_add_carry_reg8_reg16_ptr(Registers &regs, const Mmu& mmu, Reg8 r1, Reg16 r2) {
  regs.at(r1) = instr_add8(regs, regs.get(r1), mmu.read8(regs.get(r2)), regs.get(Flag::C));
}

inline void instr_add_carry_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), mmu.read8(regs.get(r)), regs.get(Flag::C));
}

inline void instr_add_carry_imm8(Registers &regs, uint8_t imm) {
  regs.at(Reg8::A) = instr_add8(regs, regs.get(Reg8::A), imm, regs.get(Flag::C));
}

inline void instr_add_reg16_sp(Registers &regs, Reg16 r) {
  regs.set(r, instr_add16(regs, regs.get(r), regs.sp));
}

inline void instr_add_reg16_reg16(Registers &regs, Reg16 r1, Reg16 r2) {
  regs.set(r1, instr_add16(regs, regs.get(r1), regs.get(r2)));
}

inline void instr_add_sp_offset(Registers &regs, int8_t e) {
  uint32_t sp = regs.sp;
  uint32_t result = sp + e;

  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (sp & 0xf) + (e & 0xf) > 0xf ? 1 : 0);
  regs.set(Flag::C, (sp & 0xff) + (e & 0xff) > 0xff ? 1 : 0);

  regs.sp = result;
}

inline uint8_t instr_sub8(Registers &regs, uint8_t a, uint8_t b, uint8_t c) {
  auto half_result = static_cast<int16_t>(a & 0xf) - (b & 0xf) - c;
  auto result = static_cast<int16_t>(a) - b - c;

  regs.set(Flag::Z, (uint8_t)result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);
  regs.set(Flag::H, half_result < 0 ? 1 : 0);
  regs.set(Flag::C, result < 0 ? 1 : 0);

  return result;
}

inline void instr_sub_reg8(Registers &regs, Reg8 r) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), regs.get(r), 0);
}

inline void instr_sub_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), mmu.read8(regs.get(r)), 0);
}

inline void instr_sub_imm8(Registers &regs, uint8_t imm) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), imm, 0);
}

inline void instr_sub_carry_reg8(Registers &regs, Reg8 r) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), regs.get(r), 0);
}

inline void instr_sub_carry_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), mmu.read8(regs.get(r)), regs.get(Flag::C));
}

inline void instr_sub_carry_imm8(Registers &regs, uint8_t imm) {
  regs.at(Reg8::A) = instr_sub8(regs, regs.get(Reg8::A), imm, regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  regs.at(r1) = instr_sub8(regs, regs.get(r1), regs.get(r2), regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_imm8(Registers &regs, Reg8 r, uint8_t imm) {
  regs.at(r) = instr_sub8(regs, regs.get(r), imm, regs.get(Flag::C));
}

inline void instr_sub_carry_reg8_reg16_ptr(Registers &regs, Mmu& mmu, Reg8 r1, Reg16 r2) {
  regs.at(r1) = instr_sub8(regs, regs.get(r1), mmu.read8(regs.get(r2)), regs.get(Flag::C));
}

inline void instr_cmp_reg8(Registers &regs, Reg8 r) {
  instr_sub8(regs, regs.get(Reg8::A), regs.get(r), 0);
}

inline void instr_cmp_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  instr_sub8(regs, regs.get(Reg8::A), mmu.read8(regs.get(r)), 0);
}

inline void instr_cmp_imm8(Registers &regs, uint8_t imm) {
  instr_sub8(regs, regs.get(Reg8::A), imm, 0);
}

inline void instr_inc_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t result = val + 1;
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t result = val + 1;
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (val & 0xf) + 1 > 0xf ? 1 : 0);
}

inline void instr_inc_sp(Registers &regs) {
  regs.sp += 1;
}

inline void instr_inc_reg16(Registers &regs, Reg16 r) {
  regs.set(r, regs.get(r) + 1);
}

inline void instr_dec_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t result = val - 1;
  regs.at(r) = result;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);
  regs.set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t result = val - 1;
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);
  regs.set(Flag::H, (val & 0xf) == 0);
}

inline void instr_dec_sp(Registers &regs) {
  regs.sp -= 1;
}

inline void instr_dec_reg16(Registers &regs, Reg16 r) {
  regs.set(r, regs.get(r) - 1);
}

inline void instr_and_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) &= regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
  regs.set(Flag::C, 0);
}

inline void instr_and_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  auto result = regs.at(Reg8::A) &= mmu.read8(regs.get(r));
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
  regs.set(Flag::C, 0);
}

inline void instr_and_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) &= imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
  regs.set(Flag::C, 0);
}

inline void instr_or_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) |= regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_or_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  auto result = regs.at(Reg8::A) |= mmu.read8(regs.get(r));
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_or_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) |= imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_xor_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) ^= regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_xor_reg16_ptr(Registers &regs, const Mmu& mmu, Reg16 r) {
  auto result = regs.at(Reg8::A) ^= mmu.read8(regs.get(r));
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_xor_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) ^= imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_ccf(Registers &regs) {
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, regs.get(Flag::C) ? 0 : 1);
}

inline void instr_scf(Registers &regs) {
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 1);
}

inline void instr_daa(Registers &regs) {
  auto a = regs.get(Reg8::A);
  auto n = regs.get(Flag::N);
  auto h = regs.get(Flag::H);
  auto c = regs.get(Flag::C);

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
      regs.set(Flag::C, 1);
    }
    if (h || (a & 0x0f) > 0x09) {
      a += 0x6;
    }
  }

  regs.set(Reg8::A, a);
  regs.set(Flag::Z, a == 0 ? 1 : 0);
  regs.set(Flag::H, 0);
}

inline void instr_cpl(Registers &regs) {
  regs.set(Reg8::A, ~regs.get(Reg8::A));
  regs.set(Flag::N, 1);
  regs.set(Flag::H, 1);
}

inline void instr_rlca(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rrca(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rla(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | regs.get(Flag::C);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, 0);//result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C,  carry);
}

inline void instr_rra(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (regs.get(Flag::C) << 7);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rlc_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rlc_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rrc_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rrc_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rl_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | regs.get(Flag::C);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C,  carry);
}

inline void instr_rl_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | regs.get(Flag::C);
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C,  carry);
}

inline void instr_rr_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (regs.get(Flag::C) << 7);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rr_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (regs.get(Flag::C) << 7);
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_sla_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = val << 1;
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_sla_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = val << 1;
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_srl_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = val >> 1;
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_srl_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = val >> 1;
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_sra_reg8(Registers &regs, Reg8 r) {
  uint8_t val = regs.get(r);
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (val & 0x80);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_sra_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  uint8_t val = mmu.read8(regs.get(r));
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (val & 0x80);
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_swap_reg8(Registers &regs, Reg8 r) {
  auto val = regs.get(r);
  uint8_t upper = (val >> 4) & 0xf;
  uint8_t lower = val & 0xf;
  uint8_t result = upper | (lower << 4);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_swap_reg16_ptr(Registers &regs, Mmu& mmu, Reg16 r) {
  auto val = mmu.read8(regs.get(r));
  uint8_t upper = (val >> 4) & 0xf;
  uint8_t lower = val & 0xf;
  uint8_t result = upper | (lower << 4);
  mmu.write8(regs.get(r), result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 0);
}

inline void instr_bit_imm8_reg8(Registers &regs, uint8_t imm, Reg8 r) {
  auto bit = (regs.get(r) >> imm) & 0x1;
  regs.set(Flag::Z, bit == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
}

inline void instr_bit_imm8_reg16_ptr(Registers &regs, const Mmu& mmu, uint8_t imm, Reg16 r) {
  auto bit = (mmu.read8(regs.get(r)) >> imm) & 0x1;
  regs.set(Flag::Z, bit == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
}

inline void instr_reset_imm8_reg8(Registers &regs, uint8_t imm, Reg8 r) {
  uint8_t mask = regs.get(r) & (1 << imm);
  regs.at(r) ^= mask;
}

inline void instr_reset_imm8_reg16_ptr(Registers &regs, Mmu& mmu, uint8_t imm, Reg16 r) {
  uint8_t mask = mmu.read8(regs.get(r)) & (1 << imm);
  uint16_t addr = regs.get(r);
  uint8_t result = mmu.read8(addr) ^ mask;
  mmu.write8(addr, result);
}

inline void instr_set_imm8_reg8(Registers &regs, uint8_t imm, Reg8 r) {
  uint8_t mask = 1 << imm;
  regs.at(r) |= mask;
}

inline void instr_set_imm8_reg16_ptr(Registers &regs, Mmu& mmu, uint8_t imm, Reg16 r) {
  uint8_t mask = 1 << imm;
  uint16_t addr = regs.get(r);
  uint8_t result = mmu.read8(addr) | mask;
  mmu.write8(addr, result);
}

inline bool instr_jump_imm16(Registers &regs, uint16_t imm) {
  regs.pc = imm;
  return true;
}

inline bool instr_jump_reg16(Registers &regs, Reg16 r) {
  regs.pc = regs.get(r);
  return true;
}

inline bool instr_jump_cond_imm16(Registers &regs, Cond cond, uint16_t nn) {
  if (check_cond(regs, cond)) {
    regs.pc = nn;
    return true;
  }
  return false;
}

inline bool instr_jump_rel(Registers &regs, int8_t e) {
  regs.pc += e;
  return true;
}

inline bool instr_jump_rel_cond(Registers &regs, Cond cond, int8_t e) {
  if (check_cond(regs, cond)) {
    regs.pc += e;
    return true;
  }
  return false;
}

inline bool instr_call_imm16(Registers &regs, Mmu& mmu, uint16_t nn) {
  Stack::push16(mmu, regs.sp, regs.pc);
  regs.pc = nn;
  return true;
}

inline bool instr_call_cond_imm16(Registers &regs, Mmu& mmu, Cond cond, uint16_t nn) {
  if (check_cond(regs, cond)) {
    Stack::push16(mmu, regs.sp, regs.pc);
    regs.pc = nn;
    return true;
  }
  return false;
}

inline bool instr_ret(Registers &regs, Mmu& mmu) {
  regs.pc = Stack::pop16(mmu, regs.sp);
  return true;
}

inline bool instr_ret_cond(Registers &regs, Mmu& mmu, Cond cond) {
  if (check_cond(regs, cond)) {
    regs.pc = Stack::pop16(mmu, regs.sp);
    return true;
  }
  return false;
}

inline void instr_reti(Registers &regs, State &state, Mmu& mmu) {
  regs.pc = Stack::pop16(mmu, regs.sp);
  state.ime = true;
}

inline void instr_restart(Registers &regs, Mmu& mmu, uint8_t n) {
  Stack::push16(mmu, regs.sp, regs.pc);
  regs.pc = n;
}

inline void execute_ld(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_SP_Reg16 &operands) { instr_load_sp_reg16(cpu.regs, operands.reg); },
     [&](Operands_SP_Imm16 &operands) { instr_load_sp_imm16(cpu.regs, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_load_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_load_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg16_Reg16 &operands) { instr_load_reg16_reg16(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Imm16 &operands) { instr_load_reg16_imm16(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu.regs, mmu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_load_reg8_reg16_ptr(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Inc &operands) { instr_load_reg8_reg16_ptr_inc(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Dec &operands) { instr_load_reg8_reg16_ptr_dec(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { instr_load_reg8_imm16_ptr(cpu.regs, mmu, operands.reg, operands.addr); },
     [&](Operands_Reg16_SP_Offset &operands) { instr_load_reg16_sp_offset(cpu.regs, operands.reg, operands.offset); },
     [&](Operands_Reg16_Ptr_Reg8 &operands) { instr_load_reg16_ptr_reg8(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Inc_Reg8 &operands) { instr_load_reg16_ptr_inc_reg8(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { instr_load_reg16_ptr_imm8(cpu.regs, mmu, operands.reg, operands.imm); },
     [&](Operands_Reg16_Ptr_Dec_Reg8 &operands) { instr_load_reg16_ptr_dec_reg8(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { instr_load_imm16_ptr_reg8(cpu.regs, mmu, operands.addr, operands.reg); },
     [&](Operands_Imm16_Ptr_SP &operands) { instr_load_imm16_ptr_sp(cpu.regs, mmu, operands.addr); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

inline void execute_inc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_inc_reg8(cpu.regs, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_inc_reg16(cpu.regs, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_inc_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](Operands_SP &operands) { instr_inc_sp(cpu.regs); },
     [&](auto&) { std::unreachable(); },
  }, instr.operands);
}

void execute_dec(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_dec_reg8(cpu.regs, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_dec_reg16(cpu.regs, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_dec_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](Operands_SP &operands) { instr_dec_sp(cpu.regs); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_add(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_add_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_reg8_reg16_ptr(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Reg16 &operands) { instr_add_reg16_reg16(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_SP_Offset &operands) { instr_add_sp_offset(cpu.regs, operands.offset); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](Operands_Reg16_SP &operands) { instr_add_reg16_sp(cpu.regs, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_adc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_carry_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_carry_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_carry_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_carry_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_add_carry_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_carry_reg8_reg16_ptr(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sub(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sbc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_carry_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_carry_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_carry_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_sub_carry_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_sub_carry_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_sub_carry_reg8_reg16_ptr(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_and(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_and_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_and_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_and_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_xor(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_xor_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_xor_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_xor_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_or(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_or_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_or_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_or_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_cp(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_cmp_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_cmp_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_cmp_reg16_ptr(cpu.regs, mmu, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_rlca(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rlca(cpu.regs);
}

void execute_rrca(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rrca(cpu.regs);
}

void execute_rla(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rla(cpu.regs);
}

void execute_rra(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_rra(cpu.regs);
}

void execute_daa(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_daa(cpu.regs);
}

void execute_cpl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_cpl(cpu.regs);
}

void execute_scf(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_scf(cpu.regs);
}

void execute_ccf(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_ccf(cpu.regs);
}

void execute_jr(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Offset &operands) { instr_jump_rel(cpu.regs, operands.offset); },
     [&](Operands_Cond_Offset &operands) { instr_jump_rel_cond(cpu.regs, operands.cond, operands.offset); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_halt(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.halt = true;
}

void execute_ldh(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8_Imm8_Ptr &operands) { instr_load_hi_reg8_imm8_ptr(cpu.regs, mmu, operands.reg, operands.addr); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu.regs, mmu, operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg8_Ptr &operands) { instr_load_hi_reg8_reg8_ptr(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Ptr_Reg8 &operands) { instr_load_hi_reg8_ptr_reg8(cpu.regs, mmu, operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_pop(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_pop_reg16(cpu.regs, mmu, operands.reg);
}

void execute_push(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_push_reg16(cpu.regs, mmu, operands.reg);
}

void execute_rst(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  auto operands = std::get<Operands_Imm8_Literal>(instr.operands);
  instr_restart(cpu.regs, mmu, operands.imm);
}

bool execute_call(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm16>(&instr.operands)) {
    return instr_call_imm16(cpu.regs,  mmu, ops->imm);
  } else if (const auto *ops = std::get_if<Operands_Cond_Imm16>(&instr.operands)) {
    return instr_call_cond_imm16(cpu.regs, mmu, ops->cond, ops->imm);
  }
  std::unreachable();
}

bool execute_jp(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  return std::visit(overloaded{
    [&](Operands_Reg16 &operands) { return instr_jump_reg16(cpu.regs, operands.reg); },
    [&](Operands_Imm16 &operands) { return instr_jump_imm16(cpu.regs, operands.imm); },
    [&](Operands_Cond_Imm16 &operands) { return instr_jump_cond_imm16(cpu.regs, operands.cond, operands.imm); },
    [&](auto &) { std::unreachable(); return false; },
  }, instr.operands);
}

bool execute_ret(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (std::get_if<Operands_None>(&instr.operands)) {
    return instr_ret(cpu.regs,  mmu);
  } else if (const auto *ops = std::get_if<Operands_Cond>(&instr.operands)) {
    return instr_ret_cond(cpu.regs, mmu, ops->cond);
  }
  std::unreachable();
}

void execute_reti(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  instr_reti(cpu.regs, cpu.state, mmu);
}

void execute_rlc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rlc_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rlc_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_rrc(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rrc_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rrc_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_rl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rl_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rl_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_rr(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rr_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_rr_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_sla(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sla_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sla_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_sra(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sra_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_sra_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_srl(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_srl_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_srl_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_swap(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_swap_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16_Ptr>(&instr.operands)) {
    instr_swap_reg16_ptr(cpu.regs, mmu, ops->reg);
  }
}

void execute_bit(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_bit_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_bit_imm8_reg16_ptr(cpu.regs, mmu, ops->imm, ops->reg);
  }
}

void execute_res(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_reset_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_reset_imm8_reg16_ptr(cpu.regs, mmu, ops->imm, ops->reg);
  }
}

void execute_set(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_set_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_set_imm8_reg16_ptr(cpu.regs, mmu, ops->imm, ops->reg);
  }
}

void execute_di(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  spdlog::info("execute_di");
  cpu.state.ime = false;
}

void execute_ei(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  spdlog::info("execute_ei");
  cpu.state.ime = true;
}

void execute_stop(Cpu &cpu, Mmu& mmu, Instruction &instr) {
  cpu.state.stop = true;
  mmu.write8(std::to_underlying(IO::DIV), 0);
}

uint8_t Cpu::execute() {
  auto interrupt_cycles = execute_interrupts();
  if (interrupt_cycles) {
    return interrupt_cycles;
  }

  if (state.halt) {
    return 4;
  }

  Instruction instr = Decoder::decode(read_next8());

  if (instr.opcode == Opcode::PREFIX) {
    instr = Decoder::decode_prefixed(read_next8());
  }

  std::visit(overloaded{
     [&](Operands_Imm8 &operands) { operands.imm = read_next8(); },
     [&](Operands_Imm16 &operands) { operands.imm = read_next16(); },
     [&](Operands_Offset &operands) { operands.offset = static_cast<int8_t>(read_next8()); },
     [&](Operands_Reg8_Imm8 &operands) { operands.imm = read_next8(); },
     [&](Operands_Reg16_Imm16 &operands) { operands.imm =read_next16(); },
     [&](Operands_Cond_Imm16 &operands) { operands.imm = read_next16(); },
     [&](Operands_Cond_Offset &operands) { operands.offset = static_cast<int8_t>(read_next8()); },
     [&](Operands_SP_Imm16 &operands) { operands.imm = read_next16(); },
     [&](Operands_SP_Offset &operands) { operands.offset = static_cast<int8_t>(read_next8()); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { operands.addr = read_next16(); },
     [&](Operands_Imm16_Ptr_SP &operands) { operands.addr = read_next16(); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { operands.addr = read_next8(); },
     [&](Operands_Reg8_Imm8_Ptr &operands) { operands.addr = read_next8(); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { operands.addr = read_next16(); },
     [&](Operands_Reg16_SP_Offset &operands) { operands.offset = static_cast<int8_t>(read_next8()); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { operands.imm = read_next8(); },
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
  case Opcode::JR: execute_jr(*this, mmu, instr); break;
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

uint8_t Cpu::execute_interrupts() {
  if (!state.ime && !state.halt) {
    return 0;
  }

  auto enable = mmu.read8(std::to_underlying(IO::IE));
  auto flag = mmu.read8(std::to_underlying(IO::IF));

  if (!(enable & flag)) {
    return 0;
  }

  state.halt = false;
  if (!state.ime) {
    return 0;
  }

  for (int i = 0; i < std::to_underlying(Interrupt::Count); i++) {
    Interrupt interrupt { i };
    if (interrupts.is_requested(interrupt)) {
      spdlog::debug("Handling interrupt: {}", magic_enum::enum_name(interrupt));
      state.ime = false;
      interrupts.clear_interrupt(interrupt);

      auto handler_addr = interrupt_handler(interrupt);
      instr_call_imm16(regs, mmu, handler_addr);

      return 20;
    }
  }

  return 0;
}

uint8_t Cpu::read_next8() {
  auto result = mmu.read8(regs.pc);
  regs.pc += 1;
  return result;
}

uint16_t Cpu::read_next16() {
  auto result = mmu.read16(regs.pc);
  regs.pc += 2;
  return result;
}

void Cpu::reset() {
  regs.reset();
  state.reset();
}

Cpu::Cpu(Mmu &mmu_, InterruptDevice &interrupts_):mmu{mmu_}, interrupts{interrupts_} {}
