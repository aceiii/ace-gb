#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <cstdint>
#include <ranges>
#include <utility>
#include <spdlog/spdlog.h>

#include "memory.h"

enum class Flag {
  C = 4,
  H = 5,
  N = 6,
  Z = 7,
};

enum class Reg8 {
  A = 0,
  F,
  B,
  C,
  D,
  E,
  H,
  L,
  Count,
};

enum class Reg16 {
  AF = 0,
  BC = 2,
  DE = 4,
  HL = 6,
};

struct Registers {
  std::array<uint8_t, std::to_underlying(Reg8::Count)> vals {};
  uint16_t pc {};
  uint16_t sp {};

  inline uint8_t& at(Reg8 reg) {
    return vals[std::to_underlying(reg)];
  }

  [[nodiscard]] inline uint8_t get(Reg8 reg) const {
    return vals[std::to_underlying(reg)];
  }

  inline void set(Reg8 reg, uint8_t val) {
    vals[std::to_underlying(reg)] = val & (reg == Reg8::F ? 0xf0 : 0xff);
  }

  [[nodiscard]] inline uint16_t get(Reg16 reg) const {
    int idx = std::to_underlying(reg);
    return (vals[idx] << 8) | vals[idx + 1];
  }

  inline void set(Reg16 reg, uint16_t val) {
    int idx = std::to_underlying(reg);
    vals[idx] = val >> 8;
    vals[idx + 1] = val & (reg == Reg16::AF ? 0xf0 : 0xff);
  }

  [[nodiscard]] uint8_t get(Flag flag) const {
    return (vals[std::to_underlying(Reg8::F)] >> std::to_underlying(flag)) & 1;
  }

  inline void set(Flag flag, uint8_t bit) {
    auto& val = vals[std::to_underlying(Reg8::F)];
    val = (val & ~(1 << std::to_underlying(flag))) | ((bit & 1) << std::to_underlying(flag));
  }

  inline void set_flags(uint8_t flag_bits) {
    auto &val = vals[std::to_underlying(Reg8::F)];
    val = (flag_bits & 0xf) << 4;
  }

  inline void reset() {
    vals.fill(0);
    sp = 0;
    pc = 0;
  }
};

inline bool operator==(const Registers &r1, const Registers &r2) {
  return std::ranges::equal(r1.vals, r2.vals) && r1.sp == r2.sp && r1.pc == r2.pc;
}

template <>
struct std::formatter<Registers> {
public:
  constexpr auto parse(auto &ctx) {
    return ctx.begin();
  }

  auto format(const Registers &regs, auto &ctx) const {
    return std::format_to(ctx.out(), "a={}, f={}, b={}, c={}, d={}, e={}, h={}, l={}, sp={}, pc={}",
                          regs.get(Reg8::A), regs.get(Reg8::F), regs.get(Reg8::B), regs.get(Reg8::C), regs.get(Reg8::D),
                          regs.get(Reg8::E), regs.get(Reg8::H), regs.get(Reg8::L), regs.sp, regs.pc);
  }
};
