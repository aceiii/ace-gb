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
  virtual ~AudioChannel() = default;

  virtual void Reset() = 0;
  virtual void PowerOff() = 0;
  virtual void Write(AudioRegister reg, uint8_t value) = 0;
  virtual uint8_t Read(AudioRegister reg) const = 0;
  virtual float Sample() const = 0;
  virtual void Tick() = 0;
  virtual void Trigger() = 0;
  virtual bool IsEnabled() const = 0;

  void Clock(uint8_t sequence) {
    if (sequence == 7) {
      TickEvenlope();
    } else if (sequence == 2 || sequence == 6) {
      TickLength();
      TickSweep();
    } else if (sequence == 0 || sequence == 4) {
      TickLength();
    }
  }

private:
  virtual void TickLength() = 0;
  virtual void TickEvenlope() = 0;
  virtual void TickSweep() = 0;
};
