#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <cstdint>
#include <ranges>
#include <utility>
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>


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
  std::array<u8, std::to_underlying(Reg8::Count)> vals {};
  u16 pc {};
  u16 sp {};

  u8& At(Reg8 reg) {
    ZoneScoped;
    return vals[std::to_underlying(reg)];
  }

  [[nodiscard]] u8 Get(Reg8 reg) const {
    ZoneScoped;
    return vals[std::to_underlying(reg)];
  }

  void Set(Reg8 reg, u8 val) {
    ZoneScoped;
    vals[std::to_underlying(reg)] = val & (reg == Reg8::F ? 0xf0 : 0xff);
  }

  [[nodiscard]] u16 Get(Reg16 reg) const {
    ZoneScoped;
    int idx = std::to_underlying(reg);
    return (vals[idx] << 8) | vals[idx + 1];
  }

  void Set(Reg16 reg, u16 val) {
    ZoneScoped;
    int idx = std::to_underlying(reg);
    vals[idx] = val >> 8;
    vals[idx + 1] = val & (reg == Reg16::AF ? 0xf0 : 0xff);
  }

  [[nodiscard]] u8 Get(Flag flag) const {
    ZoneScoped;
    return (vals[std::to_underlying(Reg8::F)] >> std::to_underlying(flag)) & 1;
  }

  void Set(Flag flag, u8 bit) {
    ZoneScoped;
    auto& val = vals[std::to_underlying(Reg8::F)];
    val = (val & ~(1 << std::to_underlying(flag))) | ((bit & 1) << std::to_underlying(flag));
  }

  void SetFlags(u8 flag_bits) {
    ZoneScoped;
    auto& val = vals[std::to_underlying(Reg8::F)];
    val = (flag_bits & 0xf) << 4;
  }

  void Reset() {
    ZoneScoped;
    vals.fill(0);
    sp = 0;
    pc = 0;
  }
};

inline bool operator==(const Registers& r1, const Registers& r2) {
  return std::ranges::equal(r1.vals, r2.vals) && r1.sp == r2.sp && r1.pc == r2.pc;
}

template <>
struct std::formatter<Registers> {
public:
  constexpr auto parse(auto& ctx) {
    return ctx.begin();
  }

  auto format(const Registers& regs, auto& ctx) const {
    ZoneScoped;
    return std::format_to(ctx.out(), "a={}, f={}, b={}, c={}, d={}, e={}, h={}, l={}, sp={}, pc={}",
                          regs.Get(Reg8::A), regs.Get(Reg8::F), regs.Get(Reg8::B), regs.Get(Reg8::C), regs.Get(Reg8::D),
                          regs.Get(Reg8::E), regs.Get(Reg8::H), regs.Get(Reg8::L), regs.sp, regs.pc);
  }
};
