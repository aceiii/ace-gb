#include "cpu.h"

#include <variant>

template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void instr_add8(uint8_t &dst, uint8_t &src, Registers& regs) {
  dst = dst + src;
  regs.set(Flag::Z, dst == 0);
  regs.set(Flag::N, 0);
}

void CPU::execute() {
  Instruction instr = Decoder::decode(memory.get(), regs.pc);

  uint8_t *dst_ptr = nullptr;
  uint8_t *src_ptr = nullptr;

  bool has_operands = instr.operands.has_value();
  if (has_operands) {
    auto operands = instr.operands.value();

    std::visit(overloaded {
        [&, dst=operands.dst] (const Reg8 &reg) {
          if (dst.immediate) {
            dst_ptr = &regs.at(reg);
          } else {
            dst_ptr = &memory[regs.get(reg)];
          }
        },
        [] (auto&) {}
    }, operands.dst.op);
  }

  switch (instr.opcode) {
  case Opcode::LD: instr_add8(*dst_ptr, *src_ptr, regs);  break;
  default:
    break;
  }
}

void CPU::run() {
}
