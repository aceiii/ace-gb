#pragma once

#include <array>

#include "audio_channel.h"

class NoiseChannel : public AudioChannel {
public:
  explicit NoiseChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  uint8_t sample() const override;
  void clock(uint8_t sequence) override;
  void trigger() override;

private:
  bool enable_channel {};

  std::array<uint8_t, 5> masks;

  union {
    std::array<uint8_t, 5> regs {};

    struct {
      uint8_t nrx0; // unused

      struct {
        uint8_t initial_length_timer: 6;
        uint8_t : 2;
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

      struct {
        uint8_t clock_divider : 3;
        uint8_t lfsr_width : 1;
        uint8_t clock_shift : 4;
      } nrx3;

      struct {
        uint8_t : 6;
        uint8_t length_enable: 1;
        uint8_t trigger: 1;
      } nrx4;
    };
  };
};
