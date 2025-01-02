#include <cassert>
#include <tuple>
#include <spdlog/spdlog.h>

#include "cpu.h"
#include "opcodes.h"
#include "overloaded.h"
#include "instructions.h"

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

inline void instr_load_reg8_reg16_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, Reg16 r2) {
  regs.at(r1) = mem[regs.get(r2)];
}

inline void instr_load_reg16_ptr_reg8(Registers &regs, uint8_t *mem, Reg16 r1, Reg8 r2) {
  mem[regs.get(r1)] = regs.get(r2);
}

inline void instr_load_reg16_ptr_imm8(Registers &regs, uint8_t *mem, Reg16 r1, uint8_t val) {
  mem[regs.get(r1)] = val;
}

inline void instr_load_reg8_imm16_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, uint16_t val) {
  regs.at(r1) = mem[val];
}

inline void instr_load_imm16_ptr_reg8(Registers &regs, uint8_t *mem, uint16_t val, Reg8 r1) {
  mem[val] = regs.get(r1);
}

inline void instr_load_hi_reg8_reg8_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, Reg8 r2) {
  regs.at(r1) = mem[0xff00 | regs.get(r2)];
}

inline void instr_load_hi_reg8_ptr_reg8(Registers &regs, uint8_t *mem, Reg8 r1, Reg8 r2) {
  mem[0xff00 | regs.get(r1)] = regs.get(r2);
}

inline void instr_load_hi_reg8_imm8_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, uint8_t val) {
  regs.at(r1) = mem[0xff00 | val];
}

inline void instr_load_hi_imm8_ptr_reg8(Registers &regs, uint8_t *mem, uint8_t val, Reg8 r1) {
  mem[0xff00 | val] = regs.get(r1);
}

inline void instr_load_reg8_reg16_ptr_dec(Registers &regs, const uint8_t *mem, Reg8 r1, Reg16 r2) {
  auto addr = regs.get(r2);
  regs.at(r1) = mem[addr];
  regs.set(r2, addr - 1);
}

inline void instr_load_reg16_ptr_dec_reg8(Registers &regs, uint8_t *mem, Reg16 r1, Reg8 r2) {
  auto addr = regs.get(r1);
  mem[addr] = regs.get(r2);
  regs.set(r1, addr - 1);
}

inline void instr_load_reg8_reg16_ptr_inc(Registers &regs, const uint8_t *mem, Reg8 r1, Reg16 r2) {
  auto addr = regs.get(r2);
  regs.at(r1) = mem[addr];
  regs.set(r2, addr + 1);
}

inline void instr_load_reg16_ptr_inc_reg8(Registers &regs, uint8_t *mem, Reg16 r1, Reg8 r2) {
  auto addr = regs.get(r1);
  mem[addr] = regs.get(r2);
  regs.set(r1, addr + 1);
}

inline void instr_load_reg16_reg16(Registers &regs, Reg16 r1, Reg16 r2) {
  regs.set(r1, regs.get(r2));
}

inline void instr_load_reg16_imm16(Registers &regs, Reg16 r1, uint16_t val) {
  regs.set(r1, val);
}

inline void instr_load_imm16_ptr_sp(Registers &regs, uint8_t *mem, uint16_t val) {
  Mem::set16(mem, val, regs.sp);
}

inline void instr_load_sp_reg16(Registers &regs, Reg16 r1) {
  regs.sp = regs.get(r1);
}

inline void instr_load_sp_imm16(Registers &regs, uint16_t imm) {
  regs.sp = imm;
}

inline void instr_load_reg16_sp_offset(Registers &regs, Reg16 r1, int8_t e) {
  uint16_t result = regs.sp + e;

  regs.set(r1, result);
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xfff) < (regs.sp & 0xfff) ? 1 : 0);
  regs.set(Flag::C, result < regs.sp ? 1 : 0);
}

inline void instr_push_reg16(Registers &regs, uint8_t *mem, Reg16 r1) {
  regs.push(mem, regs.get(r1));
}

inline void instr_pop_reg16(Registers &regs, uint8_t *mem, Reg16 r1) {
  regs.set(r1, regs.pop16(mem));
}

inline void instr_add_reg8(Registers &regs, Reg8 r) {
  auto a = regs.get(Reg8::A);
  auto result = a + regs.get(r);

  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (a & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < a ? 1 : 0);
}

inline void instr_add_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto a = regs.get(Reg8::A);
  auto result = a + mem[regs.get(r)];

  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (a & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < a ? 1 : 0);
}

inline void instr_add_reg16_sp(Registers &regs, Reg16 r) {
  auto val = regs.get(r);
  auto result = val + regs.sp;
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xfff) < (val & 0xfff) ? 1 : 0);
  regs.set(Flag::C, result < val ? 1 : 0);
}

inline void instr_add_imm8(Registers &regs, uint8_t imm) {
  auto a = regs.get(Reg8::A);
  auto result = a + imm;
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (a & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < a ? 1 : 0);
}

inline void instr_add_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  auto val = regs.get(r1);
  auto result = val + regs.get(r2);
  regs.set(r1, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (val & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < val ? 1 : 0);
}

inline void instr_add_reg8_imm8(Registers &regs, Reg8 r1, uint8_t imm) {
  auto val = regs.get(r1);
  auto result = val + imm;
  regs.set(r1, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (val & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < val ? 1 : 0);
}


inline void instr_add_reg8_reg16_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, Reg16 r2) {
  auto val = regs.get(r1);
  auto result = val + mem[regs.get(r2)];
  regs.set(r1, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (val & 0xf) ? 1 : 0);
  regs.set(Flag::C, result < val ? 1 : 0);
}

inline void instr_add_carry_reg8(Registers &regs, Reg8 r) {
  auto val1 = regs.get(Reg8::A);
  auto val2 = regs.get(r);
  auto result = val1 + val2 + regs.get(Flag::C);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val1 & 0xf) || ((result & 0xf) < (val2 & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val1) || (result < val2)) ? 1 : 0);
}

inline void instr_add_carry_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  auto val1 = regs.get(r1);
  auto val2 = regs.get(r2);
  auto result = val1 + val2 + regs.get(Flag::C);
  regs.set(r1, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val1 & 0xf) || ((result & 0xf) < (val2 & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val1) || (result < val2)) ? 1 : 0);
}

inline void instr_add_carry_reg8_imm8(Registers &regs, Reg8 r, uint8_t imm) {
  auto val = regs.get(r);
  auto result = val + imm + regs.get(Flag::C);
  regs.set(r, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val & 0xf) || ((result & 0xf) < (imm & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val) || (result < imm)) ? 1 : 0);
}

inline void instr_add_carry_reg8_reg16_ptr(Registers &regs, const uint8_t *mem, Reg8 r1, Reg16 r2) {
  auto val1 = regs.get(r1);
  auto val2 = mem[regs.get(r2)];
  auto result = val1 + val2 + regs.get(Flag::C);
  regs.set(r1, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val1 & 0xf) || ((result & 0xf) < (val2 & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val1) || (result < val2)) ? 1 : 0);
}

inline void instr_add_carry_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto val1 = regs.get(Reg8::A);
  auto val2 = mem[regs.get(r)];
  auto result = val1 + val2 + regs.get(Flag::C);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val1 & 0xf) || ((result & 0xf) < (val2 & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val1) || (result < val2)) ? 1 : 0);
}

inline void instr_add_carry_imm8(Registers &regs, uint8_t imm) {
  auto val = regs.get(Reg8::A);
  auto result = val + imm + regs.get(Flag::C);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, ((result & 0xf) < (val & 0xf) || ((result & 0xf) < (imm & 0xf))) ? 1 : 0);
  regs.set(Flag::C, ((result < val) || (result < imm)) ? 1 : 0);
}

inline void instr_sub_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) -= regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) -= mem[regs.get(r)];
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) -= imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) - regs.get(r) - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) - mem[regs.get(r)] - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) - imm - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_reg8_reg8(Registers &regs, Reg8 r1, Reg8 r2) {
  auto result = regs.at(r1) = regs.get(r1) - regs.get(r2) - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_reg8_imm8(Registers &regs, Reg8 r, uint8_t imm) {
  auto result = regs.at(r) = regs.get(r) - imm - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_sub_carry_reg8_reg16_ptr(Registers &regs, uint8_t *mem, Reg8 r1, Reg16 r2) {
  auto result = regs.at(r1) = regs.get(r1) - mem[regs.get(r2)] - regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_cmp_reg8(Registers &regs, Reg8 r) {
  auto result = regs.get(Reg8::A) - regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_cmp_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.get(Reg8::A) - mem[regs.get(r)];
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_cmp_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.get(Reg8::A) - imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_inc_reg8(Registers &regs, Reg8 r) {
  auto val = regs.get(r);
  auto result = val + 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (val & 0xf) ? 1 : 0);
}

inline void instr_inc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  auto addr = regs.get(r);
  auto val = mem[addr];
  auto result = val + 1;
  mem[addr] = result;

  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (val & 0xf) ? 1 : 0);
}

inline void instr_inc_sp(Registers &regs) {
  auto sp = regs.sp;
  auto result = sp + 1;
  regs.sp = result;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xf) < (sp & 0xf) ? 1 : 0);
}

inline void instr_dec_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(r) -= 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 1);

  // TODO: figure out the carry bits
}

inline void instr_dec_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  auto result = mem[regs.get(r)] -= 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_dec_sp(Registers &regs) {
  auto result = regs.sp -= 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_and_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) &= regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
  regs.set(Flag::C, 0);
}

inline void instr_and_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) &= mem[regs.get(r)];
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

inline void instr_or_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) |= mem[regs.get(r)];
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

inline void instr_xor_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) ^= mem[regs.get(r)];
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
  regs.set(Flag::C, regs.get(Flag::C) ? 0 : 1);
}

inline void instr_scf(Registers &regs) {
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 1);
}

inline void instr_dda(Registers &regs) {
  // TODO: decimal adjust register A
//  regs.set(Flag::Z, result == 0 ? 1 : 0);
//  regs.set(Flag::H, 0);
//  regs.set(Flag::C, carry);
}

inline void instr_cpl(Registers &regs) {
  regs.set(Reg8::A, ~regs.get(Reg8::A));
  regs.set(Flag::N, 1);
  regs.set(Flag::H, 1);
}

inline void instr_inc_reg16(Registers &regs, Reg16 r) {
  regs.set(r, regs.get(r) + 1);
}

inline void instr_dec_reg16(Registers &regs, Reg16 r) {
  regs.set(r, regs.get(r) - 1);
}

inline void instr_add_reg16_reg16(Registers &regs, Reg16 r1, Reg16 r2) {
  auto val = regs.get(r1);
  auto result = val + regs.get(r2);
  regs.set(r1, result);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xfff) < (val & 0xfff) ? 1 : 0);
  regs.set(Flag::C, (result < val) ? 1 : 0);
}

inline void instr_add_sp_offset(Registers &regs, int8_t e) {
  auto sp = regs.sp;
  uint16_t result = sp + e;
  regs.sp = result;
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, (result & 0xfff) < (sp & 0xfff) ? 1 : 0);
  regs.set(Flag::C, (result < sp) ? 1 : 0);
}

inline void instr_rlca(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  regs.set(Reg8::A, result);
//  regs.set(Flag::Z, result == 0 ? 1 : 0);
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
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, carry);
}

inline void instr_rla(Registers &regs) {
  uint8_t val = regs.get(Reg8::A);
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | regs.get(Flag::C);
  regs.set(Reg8::A, result);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
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

inline void instr_rlc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | carry;
  mem[regs.get(r)] = result;
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

inline void instr_rrc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (carry << 7);
  mem[regs.get(r)] = result;
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

inline void instr_rl_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = (val << 1) | regs.get(Flag::C);
  mem[regs.get(r)] = result;
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

inline void instr_rr_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (regs.get(Flag::C) << 7);
  mem[regs.get(r)] = result;
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

inline void instr_sla_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = (val & 0x80) >> 7;
  uint8_t result = val << 1;
  mem[regs.get(r)] = result;
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

inline void instr_srl_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = val & 0x1;
  uint8_t result = val >> 1;
  mem[regs.get(r)] = result;
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

inline void instr_sra_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  uint8_t val = mem[regs.get(r)];
  uint8_t carry = val & 0x1;
  uint8_t result = (val >> 1) | (val & 0x80);
  mem[regs.get(r)] = result;
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

inline void instr_swap_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  auto val = mem[regs.get(r)];
  uint8_t upper = (val >> 4) & 0xf;
  uint8_t lower = val & 0xf;
  uint8_t result = upper | (lower << 4);
  mem[regs.get(r)] = result;
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

inline void instr_bit_imm8_reg16_ptr(Registers &regs, const uint8_t *mem, uint8_t imm, Reg16 r) {
  auto bit = (mem[regs.get(r)] >> imm) & 0x1;
  regs.set(Flag::Z, bit == 0 ? 1 : 0);
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 1);
}

inline void instr_reset_imm8_reg8(Registers &regs, uint8_t imm, Reg8 r) {
  uint8_t mask = regs.get(r) & (1 << imm);
  regs.at(r) ^= mask;
}

inline void instr_reset_imm8_reg16_ptr(Registers &regs, uint8_t *mem, uint8_t imm, Reg16 r) {
  uint8_t mask = mem[regs.get(r)] & (1 << imm);
  mem[regs.get(r)] ^= mask;
}

inline void instr_set_imm8_reg8(Registers &regs, uint8_t imm, Reg8 r) {
  uint8_t mask = 1 << imm;
  regs.at(r) |= mask;
}

inline void instr_set_imm8_reg16_ptr(Registers &regs, uint8_t *mem, uint8_t imm, Reg16 r) {
  uint8_t mask = 1 << imm;
  mem[regs.get(r)] |= mask;
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

inline bool instr_call_imm16(Registers &regs, uint8_t *mem, uint16_t nn) {
  regs.push(mem, regs.pc);
  regs.pc = nn;
  return true;
}

inline bool instr_call_cond_imm16(Registers &regs, uint8_t *mem, Cond cond, uint16_t nn) {
  if (check_cond(regs, cond)) {
    regs.push(mem, regs.pc);
    regs.pc = nn;
    return true;
  }
  return false;
}

inline bool instr_ret(Registers &regs, uint8_t *mem) {
  regs.pc = regs.pop16(mem);
  return true;
}

inline bool instr_ret_cond(Registers &regs, uint8_t *mem, Cond cond) {
  if (check_cond(regs, cond)) {
    regs.pc = regs.pop16(mem);
    return true;
  }
  return false;
}

inline void instr_reti(Registers &regs, State &state, uint8_t *mem) {
  regs.pc = regs.pop16(mem);
  state.ime = true;
}

inline void instr_restart(Registers &regs, uint8_t *mem, uint8_t n) {
  regs.push(mem, regs.pc);
  regs.pc = n;
}

inline void execute_ld(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_SP_Reg16 &operands) { instr_load_sp_reg16(cpu.regs, operands.reg); },
     [&](Operands_SP_Imm16 &operands) { instr_load_sp_imm16(cpu.regs, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_load_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_load_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg16_Reg16 &operands) { instr_load_reg16_reg16(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Imm16 &operands) { instr_load_reg16_imm16(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu.regs, cpu.memory.get(), operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_load_reg8_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Inc &operands) { instr_load_reg8_reg16_ptr_inc(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Reg16_Ptr_Dec &operands) { instr_load_reg8_reg16_ptr_dec(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { instr_load_reg8_imm16_ptr(cpu.regs, cpu.memory.get(), operands.reg, operands.addr); },
     [&](Operands_Reg16_SP_Offset &operands) { instr_load_reg16_sp_offset(cpu.regs, operands.reg, operands.offset); },
     [&](Operands_Reg16_Ptr_Reg8 &operands) { instr_load_reg16_ptr_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Inc_Reg8 &operands) { instr_load_reg16_ptr_inc_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { instr_load_reg16_ptr_imm8(cpu.regs, cpu.memory.get(), operands.reg, operands.imm); },
     [&](Operands_Reg16_Ptr_Dec_Reg8 &operands) { instr_load_reg16_ptr_dec_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { instr_load_imm16_ptr_reg8(cpu.regs, cpu.memory.get(), operands.addr, operands.reg); },
     [&](Operands_Imm16_Ptr_SP &operands) { instr_load_imm16_ptr_sp(cpu.regs, cpu.memory.get(), operands.addr); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

inline void execute_inc(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_inc_reg8(cpu.regs, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_inc_reg16(cpu.regs, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_inc_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](Operands_SP &operands) { instr_inc_sp(cpu.regs); },
     [&](auto&) { std::unreachable(); },
  }, instr.operands);
}

void execute_dec(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_dec_reg8(cpu.regs, operands.reg); },
     [&](Operands_Reg16 &operands) { instr_dec_reg16(cpu.regs, operands.reg); },
     [&](Operands_Reg16_Ptr &operands) { instr_dec_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](Operands_SP &operands) { instr_dec_sp(cpu.regs); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_add(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8& operands) { instr_add_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_reg8_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg16_Reg16 &operands) { instr_add_reg16_reg16(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_SP_Offset &operands) { instr_add_sp_offset(cpu.regs, operands.offset); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](Operands_Reg16_SP &operands) { instr_add_reg16_sp(cpu.regs, operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_adc(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_add_carry_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_add_carry_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_add_carry_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_add_carry_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_add_carry_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_add_carry_reg8_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sub(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_sbc(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_sub_carry_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_sub_carry_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_sub_carry_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](Operands_Reg8_Reg8 &operands) { instr_sub_carry_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Imm8 &operands) { instr_sub_carry_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
     [&](Operands_Reg8_Reg16_Ptr &operands) { instr_sub_carry_reg8_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_and(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_and_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_and_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_and_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_xor(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_xor_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_xor_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_xor_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_or(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_or_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_or_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_or_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_cp(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8 &operands) { instr_cmp_reg8(cpu.regs, operands.reg); },
     [&](Operands_Imm8 &operands) { instr_cmp_imm8(cpu.regs, operands.imm); },
     [&](Operands_Reg16_Ptr &operands) { instr_cmp_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_rlca(CPU &cpu, Instruction &instr) {
  instr_rlca(cpu.regs);
}

void execute_rrca(CPU &cpu, Instruction &instr) {
  instr_rrca(cpu.regs);
}

void execute_rla(CPU &cpu, Instruction &instr) {
  instr_rla(cpu.regs);
}

void execute_rra(CPU &cpu, Instruction &instr) {
  instr_rra(cpu.regs);
}

void execute_dda(CPU &cpu, Instruction &instr) {
  instr_dda(cpu.regs);
}

void execute_cpl(CPU &cpu, Instruction &instr) {
  instr_cpl(cpu.regs);
}

void execute_scf(CPU &cpu, Instruction &instr) {
  instr_scf(cpu.regs);
}

void execute_ccf(CPU &cpu, Instruction &instr) {
  instr_ccf(cpu.regs);
}

void execute_jr(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Offset &operands) { instr_jump_rel(cpu.regs, operands.offset); },
     [&](Operands_Cond_Offset &operands) { instr_jump_rel_cond(cpu.regs, operands.cond, operands.offset); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_halt(CPU &cpu, Instruction &instr) {
  // TODO: halt
}

void execute_ldh(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
     [&](Operands_Reg8_Imm8_Ptr &operands) { instr_load_hi_reg8_imm8_ptr(cpu.regs, cpu.memory.get(), operands.reg, operands.addr); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu.regs, cpu.memory.get(), operands.addr, operands.reg); },
     [&](Operands_Reg8_Reg8_Ptr &operands) { instr_load_hi_reg8_reg8_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](Operands_Reg8_Ptr_Reg8 &operands) { instr_load_hi_reg8_ptr_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
     [&](auto &) { std::unreachable(); },
  }, instr.operands);
}

void execute_pop(CPU &cpu, Instruction &instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_pop_reg16(cpu.regs, cpu.memory.get(), operands.reg);
}

void execute_push(CPU &cpu, Instruction &instr) {
  auto operands = std::get<Operands_Reg16>(instr.operands);
  instr_push_reg16(cpu.regs, cpu.memory.get(), operands.reg);
}

void execute_rst(CPU &cpu, Instruction &instr) {
  auto operands = std::get<Operands_Imm8_Literal>(instr.operands);
  instr_restart(cpu.regs, cpu.memory.get(), operands.imm);
}

void execute_call(CPU &cpu, Instruction &instr) {
//  instr_call_cond_imm16();
//  instr_call_imm16();
}

void execute_jp(CPU &cpu, Instruction &instr) {
//  instr_jump_cond_imm16();
//  instr_jump_imm16();
//  instr_jump_reg16();
}

void execute_ret(CPU &cpu, Instruction &instr) {
//  instr_ret();
//  instr_ret_cond();
}

void execute_reti(CPU &cpu, Instruction &instr) {
  instr_reti(cpu.regs, cpu.state, cpu.memory.get());
}

void execute_rlc(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rlc_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_rlc_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_rrc(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rrc_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_rrc_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_rl(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rl_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_rl_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_rr(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_rr_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_rr_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_sla(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sla_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_sla_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_sra(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_sra_reg8(cpu.regs,  ops->reg);
  } if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_sra_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_swap(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Reg8>(&instr.operands)) {
    instr_swap_reg8(cpu.regs,  ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Reg16>(&instr.operands)) {
    instr_swap_reg16_ptr(cpu.regs, cpu.memory.get(), ops->reg);
  }
  std::unreachable();
}

void execute_srl(CPU &cpu, Instruction &instr) {
//  instr_srl_reg16_ptr();
//  instr_srl_reg8();
}

void execute_bit(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_bit_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_bit_imm8_reg16_ptr(cpu.regs, cpu.memory.get(), ops->imm, ops->reg);
  }
  std::unreachable();
}

void execute_res(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_reset_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_reset_imm8_reg16_ptr(cpu.regs, cpu.memory.get(), ops->imm, ops->reg);
  }
  std::unreachable();
}

void execute_set(CPU &cpu, Instruction &instr) {
  if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg8>(&instr.operands)) {
    instr_set_imm8_reg8(cpu.regs, ops->imm, ops->reg);
  } else if (const auto *ops = std::get_if<Operands_Imm8_Literal_Reg16_Ptr>(&instr.operands)) {
    instr_set_imm8_reg16_ptr(cpu.regs, cpu.memory.get(), ops->imm, ops->reg);
  }
  std::unreachable();
}

void execute_di(CPU &cpu, Instruction &instr) {
}

void execute_ei(CPU &cpu, Instruction &instr) {
}

void execute_stop(CPU &cpu, Instruction &instr) {
}

CPU::CPU(size_t mem_size):memory(Mem::create_memory(mem_size)) {}

void CPU::execute() {
  assert(memory.get() != nullptr);

  Instruction instr = Decoder::decode(read8());

  if (instr.opcode == Opcode::PREFIX) {
    instr = Decoder::decode_prefixed(read8());
  }

  spdlog::debug("Decoded: {}", instr);

  std::visit(overloaded{
     [&](Operands_Imm8 &operands) { operands.imm = read8(); },
     [&](Operands_Imm16 &operands) { operands.imm = read16(); },
     [&](Operands_Offset &operands) { operands.offset = static_cast<int8_t>(read8()); },
     [&](Operands_Reg8_Imm8 &operands) { operands.imm = read8(); },
     [&](Operands_Reg16_Imm16 &operands) { operands.imm =read16(); },
     [&](Operands_Cond_Imm16 &operands) { operands.imm = read16(); },
     [&](Operands_Cond_Offset &operands) { operands.offset = static_cast<int8_t>(read8()); },
     [&](Operands_SP_Imm16 &operands) { operands.imm = read16(); },
     [&](Operands_SP_Offset &operands) { operands.offset = static_cast<int8_t>(read8()); },
     [&](Operands_Imm16_Ptr_Reg8 &operands) { operands.addr = read16(); },
     [&](Operands_Imm16_Ptr_SP &operands) { operands.addr = read16(); },
     [&](Operands_Imm8_Ptr_Reg8 &operands) { operands.addr = read8(); },
     [&](Operands_Reg8_Imm8_Ptr &operands) { operands.addr = read8(); },
     [&](Operands_Reg8_Imm16_Ptr &operands) { operands.addr = read16(); },
     [&](Operands_Reg16_SP_Offset &operands) { operands.offset = static_cast<int8_t>(read8()); },
     [&](Operands_Reg16_Ptr_Imm8 &operands) { operands.imm = read8(); },
     [&](auto &operands) { /* pass */ }
  }, instr.operands);

  switch (instr.opcode) {
  case Opcode::INVALID:
  case Opcode::NOP: break;
  case Opcode::LD: execute_ld(*this, instr); break;
  case Opcode::INC: execute_inc(*this, instr); break;
  case Opcode::DEC: execute_dec(*this, instr); break;
  case Opcode::ADD: execute_add(*this, instr); break;
  case Opcode::ADC: execute_adc(*this, instr); break;
  case Opcode::SUB: execute_sub(*this, instr); break;
  case Opcode::SBC: execute_sbc(*this, instr); break;
  case Opcode::AND: execute_and(*this, instr); break;
  case Opcode::XOR: execute_xor(*this, instr); break;
  case Opcode::OR: execute_or(*this, instr); break;
  case Opcode::CP: execute_cp(*this, instr); break;
  case Opcode::RLCA: execute_rlca(*this, instr); break;
  case Opcode::RRCA: execute_rrca(*this, instr); break;
  case Opcode::RLA: execute_rla(*this, instr); break;
  case Opcode::RRA: execute_rra(*this, instr); break;
  case Opcode::DAA: execute_dda(*this, instr); break;
  case Opcode::CPL: execute_cpl(*this, instr); break;
  case Opcode::SCF: execute_scf(*this, instr); break;
  case Opcode::CCF: execute_ccf(*this, instr); break;
  case Opcode::JR: execute_jr(*this, instr); break;
  case Opcode::HALT: execute_halt(*this, instr); break;
  case Opcode::LDH: execute_ldh(*this, instr); break;
  case Opcode::POP: execute_pop(*this, instr); break;
  case Opcode::PUSH: execute_push(*this, instr); break;
  case Opcode::RST: execute_rst(*this, instr); break;
  case Opcode::CALL: execute_call(*this, instr); break;
  case Opcode::JP: execute_jp(*this, instr); break;
  case Opcode::RET: execute_ret(*this, instr); break;
  case Opcode::RETI: execute_reti(*this, instr); break;
  case Opcode::RLC: execute_rlc(*this, instr); break;
  case Opcode::RRC: execute_rrc(*this, instr); break;
  case Opcode::RL: execute_rl(*this, instr); break;
  case Opcode::RR: execute_rr(*this, instr); break;
  case Opcode::SLA: execute_sla(*this, instr); break;
  case Opcode::SRA: execute_sra(*this, instr); break;
  case Opcode::SWAP: execute_swap(*this, instr); break;
  case Opcode::SRL: execute_srl(*this, instr); break;
  case Opcode::BIT: execute_bit(*this, instr); break;
  case Opcode::RES: execute_res(*this, instr); break;
  case Opcode::SET: execute_set(*this, instr); break;
  case Opcode::DI: execute_di(*this, instr); break;
  case Opcode::EI: execute_ei(*this, instr); break;
  case Opcode::STOP: execute_stop(*this, instr); break;
  case Opcode::PREFIX: break;
  }
}

void CPU::run() {
}

uint8_t CPU::read8() {
  auto result = memory[regs.pc];
  regs.pc += 1;
  return result;
}

uint16_t CPU::read16() {
  auto result = Mem::get16(memory.get(), regs.pc);
  regs.pc += 2;
  return result;
}
