#pragma once

#include "emulator.hpp"


class AssemblyViewer {
public:
  void Initialize(Emulator* emulator);
  void Draw();

private:
  std::string GetInstruction(uint16_t addr) const;
  std::string Decode(uint8_t op, uint8_t imm8, uint16_t imm16) const;
  std::string DecodePrefixed(uint8_t op) const;

  bool auto_scroll_ = true;
  Emulator* emulator_;
};
