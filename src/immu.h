#pragma once

#include <cstdint>

class IMMU {
public:
  virtual ~IMMU() = 0;

  virtual void write(uint16_t addr, uint8_t byte) = 0;
  virtual void write(uint16_t addr, uint16_t word) = 0;

  [[nodiscard]] virtual uint8_t read8(uint16_t addr) const = 0;
  [[nodiscard]] virtual uint16_t read16(uint16_t addr) const = 0;

  virtual void inc(uint16_t addr) = 0;

  virtual void reset() = 0;
};
