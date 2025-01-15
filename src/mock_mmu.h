#pragma once

#include <array>

#include "immu.h"

class MockMMU : public IMMU {
public:
  void load_boot_rom(const uint8_t *rom) override;
  void load_cartridge(const std::vector<uint8_t> &cart) override;

  void write(uint16_t addr, uint8_t byte) override;
  void write(uint16_t addr, uint16_t word) override;

  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  [[nodiscard]] uint16_t read16(uint16_t addr) const override;

  void inc(uint16_t addr) override;

  void on_write8(uint16_t addr, mmu_callback callback) override;

  void reset() override;

private:
  std::array<uint8_t, 1 << 16> memory;
};
