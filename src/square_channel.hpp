#pragma once

#include <array>

#include "audio_channel.hpp"


class SquareChannel : public AudioChannel {
public:
  explicit SquareChannel(bool sweep);

  void Reset() override;
  void PowerOff() override;
  void Write(AudioRegister reg, u8 value) override;
  u8 Read(AudioRegister reg) const override;
  float Sample() const override;
  void Tick() override;
  void Trigger() override;
  bool IsEnabled() const override;

private:
  void TickLength() override;
  void TickEnvenlope() override;
  void TickSweep() override;

  u16 CalcSweep();
  u16 GetFrequency() const;
  void SetFrequency(u16 freq);

private:
  bool enable_sweep_ {};
  bool enable_channel_ {};
  u16 length_counter_ {};
  u16 envelope_timer_ {};
  u16 timer_ {};
  u8 volume_ {};
  u8 duty_step_ {};

  struct {
    bool enabled;
    bool calculated;
    u16 timer;
    u16 current;
  } period_ {};

  std::array<u8, 5> masks_;

  union {
    std::array<u8, 5> regs {};

    struct {
      struct {
        u8 period_sweep_step: 3;
        u8 period_sweep_direction: 1;
        u8 period_sweep_pace: 3;
        u8 : 1;
      } nrx0;

      struct {
        u8 initial_length_timer: 6;
        u8 wave_duty: 2;
      } nrx1;

      union {
        struct {
          u8 : 3;
          u8 dac: 5;
        };
        struct {
          u8 envelope_sweep_pace: 3;
          u8 envelope_direction: 1;
          u8 initial_volume: 4;
        };
      } nrx2;

      u8 nrx3;

      struct {
        u8 period: 3;
        u8 : 3;
        u8 length_enable: 1;
        u8 trigger: 1;
      } nrx4;
    };
  };
};
