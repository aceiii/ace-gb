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
  default: [[unlikely]]
    return false;
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

  // TODO: figure out the carry bits
}

inline void instr_push_reg16(Registers &regs, uint8_t *mem, Reg16 r1) {
  regs.push(mem, regs.get(r1));
}

inline void instr_pop_reg16(Registers &regs, uint8_t *mem, Reg16 r1) {
  regs.set(r1, regs.pop16(mem));
}

inline void instr_add_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) += regs.get(r);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_add_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) += mem[regs.get(r)];
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_add_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) += imm;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_add_carry_reg8(Registers &regs, Reg8 r) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) + regs.get(r) + regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_add_carry_reg16_ptr(Registers &regs, const uint8_t *mem, Reg16 r) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) + mem[regs.get(r)] + regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_add_carry_imm8(Registers &regs, uint8_t imm) {
  auto result = regs.at(Reg8::A) = regs.get(Reg8::A) + imm + regs.get(Flag::C);
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
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
  auto result = regs.at(r) += 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
}

inline void instr_inc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  auto result = mem[regs.get(r)] += 1;
  regs.set(Flag::Z, result == 0 ? 1 : 0);
  regs.set(Flag::N, 0);

  // TODO: figure out the carry bits
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
  // TODO: implement complement carry flags
}

inline void instr_scf(Registers &regs) {
  regs.set(Flag::N, 0);
  regs.set(Flag::H, 0);
  regs.set(Flag::C, 1);
}

inline void instr_dda(Registers &regs) {
  // TODO
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
  auto result = regs.get(r1) + regs.get(r2);
  regs.set(r1, result);
  regs.set(Flag::N, 0);

  // TODO: set carry flags
}

inline void instr_add_sp_offset(Registers &regs, int8_t e) {
  uint16_t result = regs.sp + e;
  regs.sp = result;
  regs.set(Flag::Z, 0);
  regs.set(Flag::N, 0);

  // TODO: set carry flags
}

inline void instr_rlca(Registers &regs) {
  // TODO
}

inline void instr_rrca(Registers &regs) {
  // TODO
}

inline void instr_rla(Registers &regs) {
  // TODO
}

inline void instr_rra(Registers &regs) {
  // TODO
}

inline void instr_rlc_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_rlc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_rrc_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_rrc_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_rl_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_rl_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_rr_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_rr_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_sla_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_sla_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_sra_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_sra_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_swap_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_swap_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_srl_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_srl_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_bit_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_bit_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_reset_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_reset_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_set_reg8(Registers &regs, Reg8 r) {
  // TODO
}

inline void instr_set_reg16_ptr(Registers &regs, uint8_t *mem, Reg16 r) {
  // TODO
}

inline void instr_jump_imm16(Registers &regs, uint16_t imm) {
  regs.pc = imm;
}

inline void instr_jump_reg16(Registers &regs, Reg16 r) {
  regs.pc = regs.get(r);
}

inline void instr_jump_cond_imm16(Registers &regs, Cond cond, uint16_t nn) {
  if (check_cond(regs, cond)) {
    regs.pc = nn;
  }
}

inline void instr_jump_rel(Registers &regs, int8_t e) {
  regs.pc += e;
}

inline void instr_jump_rel_cond(Registers &regs, Cond cond, int8_t e) {
  if (check_cond(regs, cond)) {
    regs.pc += e;
  }
}

inline void instr_call_imm16(Registers &regs, uint8_t *mem, uint16_t nn) {
  regs.push(mem, regs.pc);
  regs.pc = nn;
}

inline void instr_call_cond_imm16(Registers &regs, uint8_t *mem, Cond cond, uint16_t nn) {
  if (check_cond(regs, cond)) {
    regs.push(mem, regs.pc);
    regs.pc = nn;
  }
}

inline void instr_ret(Registers &regs, uint8_t *mem) {
  regs.pc = regs.pop16(mem);
}

inline void instr_ret_cond(Registers &regs, uint8_t *mem, Cond cond) {
  if (check_cond(regs, cond)) {
    regs.pc = regs.pop16(mem);
  }
}

inline void instr_reti(Registers &regs, State &state, uint8_t *mem) {
  regs.pc = regs.pop16(mem);
  state.ime = true;
}

inline void instr_restart(Registers &regs, uint8_t *mem, uint8_t n) {
  regs.push(mem, regs.pc);
  regs.pc = n;
}

inline void execute_load(CPU &cpu, Instruction &instr) {
  std::visit(overloaded{
         [&](Operands_Reg8_Reg8 &operands) { instr_load_reg8_reg8(cpu.regs, operands.reg1, operands.reg2); },
         [&](Operands_Reg8_Imm8 &operands) { instr_load_reg8_imm8(cpu.regs, operands.reg, operands.imm); },
         [&](Operands_Reg16_Reg16 &operands) { instr_load_reg16_reg16(cpu.regs, operands.reg1, operands.reg2); },
         [&](Operands_Reg16_Imm16 &operands) { instr_load_reg16_imm16(cpu.regs, operands.reg, operands.imm); },
         [&](Operands_SP_Reg16 &operands) { instr_load_sp_reg16(cpu.regs, operands.reg); },
         [&](Operands_SP_Imm16 &operands) { instr_load_sp_imm16(cpu.regs, operands.imm); },
         [&](Operands_Reg8_Imm16_Ptr &operands) { instr_load_reg8_imm16_ptr(cpu.regs, cpu.memory.get(), operands.reg, operands.addr); },
         [&](Operands_Imm16_Ptr_Reg8 &operands) { instr_load_imm16_ptr_reg8(cpu.regs, cpu.memory.get(), operands.addr, operands.reg); },
         [&](Operands_Imm16_Ptr_SP &operands) { instr_load_imm16_ptr_sp(cpu.regs, cpu.memory.get(), operands.addr); },
         [&](Operands_Imm8_Ptr_Reg8 &operands) { instr_load_hi_imm8_ptr_reg8(cpu.regs, cpu.memory.get(), operands.addr, operands.reg); },
         [&](Operands_Reg8_Reg16_Ptr &operands) { instr_load_reg8_reg16_ptr(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](Operands_Reg8_Reg16_Ptr_Inc &operands) { instr_load_reg8_reg16_ptr_inc(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](Operands_Reg8_Reg16_Ptr_Dec &operands) { instr_load_reg8_reg16_ptr_dec(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](Operands_Reg16_SP_Offset &operands) { instr_load_reg16_sp_offset(cpu.regs, operands.reg, operands.offset); },
         [&](Operands_Reg16_Ptr_Reg8 &operands) { instr_load_reg16_ptr_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](Operands_Reg16_Ptr_Inc_Reg8 &operands) { instr_load_reg16_ptr_inc_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](Operands_Reg16_Ptr_Imm8 &operands) { instr_load_reg16_ptr_imm8(cpu.regs, cpu.memory.get(), operands.reg, operands.imm); },
         [&](Operands_Reg16_Ptr_Dec_Reg8 &operands) { instr_load_reg16_ptr_dec_reg8(cpu.regs, cpu.memory.get(), operands.reg1, operands.reg2); },
         [&](auto &) { std::unreachable(); },
  }, instr.operands);
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
  case Opcode::NOP:
    break;
  case Opcode::LD:
    execute_load(*this, instr);
    break;
  case Opcode::INC:
  case Opcode::DEC:
  case Opcode::ADD:
  case Opcode::ADC:
  case Opcode::SUB:
  case Opcode::SBC:
  case Opcode::AND:
  case Opcode::XOR:
  case Opcode::OR:
  case Opcode::CP:
  case Opcode::RLCA:
  case Opcode::RRCA:
  case Opcode::RLA:
  case Opcode::RRA:
  case Opcode::DAA:
  case Opcode::CPL:
  case Opcode::SCF:
  case Opcode::CCF:
  case Opcode::JR:
  case Opcode::HALT:
  case Opcode::LDH:
  case Opcode::POP:
  case Opcode::PUSH:
  case Opcode::RST:
  case Opcode::CALL:
  case Opcode::JP:
  case Opcode::RET:
  case Opcode::RETI:
  case Opcode::RLC:
  case Opcode::RRC:
  case Opcode::RL:
  case Opcode::RR:
  case Opcode::SLA:
  case Opcode::SRA:
  case Opcode::SWAP:
  case Opcode::SRL:
  case Opcode::BIT:
  case Opcode::RES:
  case Opcode::SET:
  case Opcode::DI:
  case Opcode::EI:
  case Opcode::STOP:
  case Opcode::PREFIX:
    break;
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
