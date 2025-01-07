#pragma once

#include <cstdint>
#include <functional>

typedef std::function<uint8_t(uint16_t, uint8_t)> mmu_callback;

class IMMU {
public:
  virtual ~IMMU() = 0;

  virtual void load_boot_rom(const uint8_t *rom) = 0;
  virtual void load_cartridge(std::vector<uint8_t> cart) = 0;

  virtual void write(uint16_t addr, uint8_t byte) = 0;
  virtual void write(uint16_t addr, uint16_t word) = 0;

  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint16_t read16(uint16_t addr) const = 0;

  virtual void inc(uint16_t addr) = 0;

  virtual void on_write8(uint16_t addr, mmu_callback callback) = 0;

  virtual void reset() = 0;
};
