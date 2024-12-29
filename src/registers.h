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

struct Flags {
  uint8_t &val;

  inline uint8_t get(Flag flag) const {
    return (val >> static_cast<int>(flag)) & 1;
  }

  inline void set(Flag flag, uint8_t bit) {
    val = (val & ~(1 << static_cast<int>(flag))) | ((bit & 1) << static_cast<int>(flag));
  }
};

struct Registers {
  std::array<uint8_t, std::to_underlying(Reg8::Count)> vals;
  uint16_t pc;
  uint16_t sp;

  Flags flags = { vals[std::to_underlying(Reg8::F)] };

  inline uint8_t& at(Reg8 reg) {
    return vals[std::to_underlying(reg)];
  }

  inline uint8_t& get(Reg8 reg) {
    return vals[std::to_underlying(reg)];
  }

  inline void set(Reg8 reg, uint8_t val) {
    vals[std::to_underlying(reg)] = val & (reg == Reg8::F ? 0xf0 : 0xff);
  }

  inline uint16_t get(Reg16 reg) const {
    int idx = std::to_underlying(reg);
    return (vals[idx] << 8) | vals[idx + 1];
  }

  inline void set(Reg16 reg, uint16_t val) {
    int idx = std::to_underlying(reg);
    vals[idx] = val >> 8;
    vals[idx + 1] = val & (reg == Reg16::AF ? 0xf0 : 0xff);
  }

  inline void reset() {
    vals.fill(0);
  }
};
