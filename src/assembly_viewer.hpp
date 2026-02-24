#pragma once

#include "types.hpp"
#include "emulator.hpp"


class AssemblyViewer {
public:
  void Initialize(Emulator* emulator);
  void Draw();

private:
  std::string GetInstruction(u16 addr) const;
  std::string Decode(u8 op, u8 imm8, u16 imm16) const;
  std::string DecodePrefixed(u8 op) const;

  bool auto_scroll_ = true;
  Emulator* emulator_;
};
