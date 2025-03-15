#pragma once

#include <array>

#include "audio_channel.h"

class WaveChannel : public AudioChannel {
public:
  explicit WaveChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  float sample() const override;
  void tick() override;
  void trigger() override;

  uint8_t read_wave(uint8_t idx) const;
  void set_wave(uint8_t idx, uint8_t byte);

private:
  void length_tick() override;
  void envelope_tick() override;
  void sweep_tick() override;

  uint16_t frequency() const;
  void set_frequency(uint16_t freq);

private:
  bool enable_channel {};
  uint16_t length_counter {};
  uint16_t timer {};
  uint8_t volume {};
  uint8_t wave_index {};
  uint8_t buffer {};

  std::array<uint8_t, 16> wave_pattern_ram;
  std::array<uint8_t, 5> masks;

  union {
    std::array<uint8_t, 5> regs {};

    struct {
      struct {
        uint8_t : 7;
        uint8_t dac : 1;
      } nrx0;

      struct {
        uint8_t initial_length_timer : 8;
      } nrx1;

      struct {
        uint8_t : 5;
        uint8_t output_level : 2;
        uint8_t : 1;
      } nrx2;

      uint8_t nrx3;

      struct {
        uint8_t period : 3;
        uint8_t : 3;
        uint8_t length_enable : 1;
        uint8_t trigger : 1;
      } nrx4;
    };
  };
};
