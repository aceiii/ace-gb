#pragma once

#include <array>

#include "audio_channel.h"

class SquareChannel : public AudioChannel {
public:
  explicit SquareChannel(bool sweep);

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  uint8_t sample() const override;
  void clock() override;

private:
  bool enable_sweep {};
  bool enable_channel {};

  std::array<uint8_t, 5> masks;

  union {
    std::array<uint8_t, 5> regs {};

    struct {
      struct {
        uint8_t step: 3;
        uint8_t direction: 1;
        uint8_t pace: 3;
        uint8_t : 1;
      } nrx0;

      struct {
        uint8_t initial_length_timer: 6;
        uint8_t wave_duty: 2;
      } nrx1;

      union {
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

      struct {
        uint8_t period: 3;
        uint8_t : 3;
        uint8_t length_enable: 1;
        uint8_t trigger: 1;
      } nrx4;
    };
  };
};
