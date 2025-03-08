#pragma once

#include "audio_channel.h"

class SquareChannel : public AudioChannel {
public:
  explicit SquareChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  uint8_t sample() const override;
  void tick(uint8_t cycles) override;

private:
  union {
    uint8_t val;
    struct {
      uint8_t step: 3;
      uint8_t direction: 1;
      uint8_t pace: 3;
      uint8_t unused: 1;
    };
  } nrx0;

  union {
    uint8_t val;
    struct {
      uint8_t initial_length_timer: 6;
      uint8_t wave_duty: 2;
    };
  } nrx1;

  union {
    uint8_t val;
    struct {
      uint8_t : 3;
      uint8_t dac: 5;
    };
    struct {
      uint8_t sweep_pace: 3;
      uint8_t env_dir: 1;
      uint8_t initial_volume: 4;
    };
  } nrx2;

  uint8_t nrx3;

  union {
    uint8_t val;
    struct {
      uint8_t period: 3;
      uint8_t unused: 3;
      uint8_t length_enable: 1;
      uint8_t trigger: 1;
    };
  } nrx4;
};
