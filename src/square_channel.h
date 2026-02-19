#pragma once

#include <array>

#include "audio_channel.hpp"

class SquareChannel : public AudioChannel {
public:
  explicit SquareChannel(bool sweep);

  void Reset() override;
  void PowerOff() override;
  void Write(AudioRegister reg, uint8_t value) override;
  uint8_t Read(AudioRegister reg) const override;
  float Sample() const override;
  void Tick() override;
  void Trigger() override;
  bool IsEnabled() const override;

private:
  void TickLength() override;
  void TickEvenlope() override;
  void TickSweep() override;

  uint16_t calc_sweep();
  uint16_t frequency() const;
  void set_frequency(uint16_t freq);

private:
  bool enable_sweep {};
  bool enable_channel {};
  uint16_t length_counter {};
  uint16_t envelope_timer {};
  uint16_t timer {};
  uint8_t volume {};
  uint8_t duty_step {};

  struct {
    bool enabled;
    bool calculated;
    uint16_t timer;
    uint16_t current;
  } period {};

  std::array<uint8_t, 5> masks;

  union {
    std::array<uint8_t, 5> regs {};

    struct {
      struct {
        uint8_t period_sweep_step: 3;
        uint8_t period_sweep_direction: 1;
        uint8_t period_sweep_pace: 3;
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
          uint8_t envelope_sweep_pace: 3;
          uint8_t envelope_direction: 1;
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
