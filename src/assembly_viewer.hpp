#pragma once

#include "emulator.hpp"


class AssemblyViewer {
public:
  void Initialize(Emulator* emulator);
  void Draw();

private:
  std::string GetInstruction(uint16_t addr) const;
  std::string Decode(uint8_t op) const;
  std::string DecodePrefixed(uint8_t op) const;

  bool auto_scroll_ = true;
  Emulator* emulator_;
};
