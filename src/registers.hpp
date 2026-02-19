#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <cstdint>
#include <ranges>
#include <utility>
#include <spdlog/spdlog.h>


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

  uint8_t& At(Reg8 reg) {
    return vals[std::to_underlying(reg)];
  }

  [[nodiscard]] uint8_t Get(Reg8 reg) const {
    return vals[std::to_underlying(reg)];
  }

  void Set(Reg8 reg, uint8_t val) {
    vals[std::to_underlying(reg)] = val & (reg == Reg8::F ? 0xf0 : 0xff);
  }

  [[nodiscard]] uint16_t Get(Reg16 reg) const {
    int idx = std::to_underlying(reg);
    return (vals[idx] << 8) | vals[idx + 1];
  }

  void Set(Reg16 reg, uint16_t val) {
    int idx = std::to_underlying(reg);
    vals[idx] = val >> 8;
    vals[idx + 1] = val & (reg == Reg16::AF ? 0xf0 : 0xff);
  }

  [[nodiscard]] uint8_t Get(Flag flag) const {
    return (vals[std::to_underlying(Reg8::F)] >> std::to_underlying(flag)) & 1;
  }

  void Set(Flag flag, uint8_t bit) {
    auto& val = vals[std::to_underlying(Reg8::F)];
    val = (val & ~(1 << std::to_underlying(flag))) | ((bit & 1) << std::to_underlying(flag));
  }

  void SetFlags(uint8_t flag_bits) {
    auto &val = vals[std::to_underlying(Reg8::F)];
    val = (flag_bits & 0xf) << 4;
  }

  void Reset() {
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
                          regs.Get(Reg8::A), regs.Get(Reg8::F), regs.Get(Reg8::B), regs.Get(Reg8::C), regs.Get(Reg8::D),
                          regs.Get(Reg8::E), regs.Get(Reg8::H), regs.Get(Reg8::L), regs.sp, regs.pc);
  }
};
