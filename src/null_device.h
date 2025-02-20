#pragma once

#include <map>

#include "mmu_device.h"

struct null_override {
  uint8_t value;
  bool writeable;
  uint8_t mask;
};

class NullDevice : public MmuDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void add_override(uint16_t addr, uint8_t default_value, bool writable, uint8_t mask = 0x00);

private:
  std::map<uint16_t, null_override> overrides {};
};
