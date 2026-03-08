#pragma once

#include <map>

#include "mmu_device.hpp"

struct null_override {
  u8 value;
  bool writeable;
  u8 mask;
};

class NullDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void add_override(u16 addr, u8 default_value, bool writable, u8 mask = 0x00);

private:
  std::map<u16, null_override> overrides {};
};
