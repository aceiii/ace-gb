#pragma once

#include <array>
#include <vector>

#include "mmu.h"

constexpr size_t kBootRomSize = 256;

using rom_buffer = std::array<uint8_t, kBootRomSize>;

class BootRomDevice : public MmuDevice {
public:
  void load_bytes(const rom_buffer &bytes);

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void Reset() override;

  void set_disable(uint8_t byte);

private:
  rom_buffer rom;
  uint8_t disable = 0;
};
