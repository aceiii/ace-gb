#pragma once

#include <array>

#include "audio_channel.h"

class NoiseChannel : public AudioChannel {
public:
  explicit NoiseChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  float sample() const override;
  void tick() override;
  void trigger() override;

private:
  void length_tick() override;
  void envelope_tick() override;
  void sweep_tick() override;

private:
  bool enable_channel {};
  uint16_t length_counter {};
  uint16_t envelope_timer {};
  uint16_t timer {};
  uint8_t volume {};
  union {
    uint16_t byte {};
    struct {
      uint16_t val: 15;
      uint16_t temp: 1;
    };
  } lfsr;

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
