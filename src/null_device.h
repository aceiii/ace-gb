#pragma once

#include <map>

#include "mmu_device.hpp"

struct null_override {
  uint8_t value;
  bool writeable;
  uint8_t mask;
};

class NullDevice : public MmuDevice {
public:
  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void add_override(uint16_t addr, uint8_t default_value, bool writable, uint8_t mask = 0x00);

private:
  std::map<uint16_t, null_override> overrides {};
};
