#pragma once

#include <cstdint>

enum class AudioRegister {
  NRx0 = 0,
  NRx1,
  NRx2,
  NRx3,
  NRx4,
};


class AudioChannel {
public:
  virtual void reset() = 0;
  virtual void write(AudioRegister reg, uint8_t value) = 0;
  virtual uint8_t read(AudioRegister reg) const = 0;
  virtual uint8_t sample() const = 0;
  virtual void clock() = 0;
  virtual void trigger() = 0;
};
