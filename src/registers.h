#pragma once

#include "util.h"

#include <cstdint>
#include <array>
#include <utility>

enum class Flag {
  C = 4,
  H = 5,
  N = 6,
  Z = 7,
};

enum class FlagOp {
  Unchanged = -1,
  Off = 0,
  On = 1,
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
    return (vals[std::to_underlying(Reg8::F)] >> static_cast<int>(flag)) & 1;
  }

  inline void set(Flag flag, uint8_t bit) {
    auto& val = vals[std::to_underlying(Reg8::F)];
    val = (val & ~(1 << static_cast<int>(flag))) | ((bit & 1) << static_cast<int>(flag));
  }

  inline void set_flags(uint8_t flag_bits) {
    auto &val = vals[std::to_underlying(Reg8::F)];
    val = (flag_bits & 0xf) << 4;
  }

  inline void reset() {
    vals.fill(0);
  }
};
