#pragma once

#include "mmu.h"

constexpr size_t kBootRomSize = 256;

class BootRomDevice : public MmuDevice {
public:
  void load_bytes(const std::vector<uint8_t> &bytes);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

private:
  std::array<uint8_t, kBootRomSize> rom;
  uint8_t disable = 0;
};
