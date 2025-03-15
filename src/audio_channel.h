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
  virtual float sample() const = 0;
  virtual void tick() = 0;
  virtual void trigger() = 0;

  inline void clock(uint8_t sequence) {
    if (sequence % 2 == 0) {
      length_tick();
    }
    if (sequence == 7) {
      envelope_tick();
    }
    if (sequence == 2 || sequence == 6) {
      sweep_tick();
    }
  }

private:
  virtual void length_tick() = 0;
  virtual void envelope_tick() = 0;
  virtual void sweep_tick() = 0;
};
