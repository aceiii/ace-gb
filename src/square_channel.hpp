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
  void TickEnvenlope() override;
  void TickSweep() override;

  uint16_t CalcSweep();
  uint16_t GetFrequency() const;
  void SetFrequency(uint16_t freq);

private:
  bool enable_sweep_ {};
  bool enable_channel_ {};
  uint16_t length_counter_ {};
  uint16_t envelope_timer_ {};
  uint16_t timer_ {};
  uint8_t volume_ {};
  uint8_t duty_step_ {};

  struct {
    bool enabled;
    bool calculated;
    uint16_t timer;
    uint16_t current;
  } period_ {};

  std::array<uint8_t, 5> masks_;

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
