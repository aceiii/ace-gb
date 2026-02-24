#pragma once

#include <array>

#include "types.hpp"
#include "audio_channel.hpp"


class WaveChannel : public AudioChannel {
public:
  explicit WaveChannel();

  void Reset() override;
  void PowerOff() override;
  void Write(AudioRegister reg, u8 value) override;
  u8 Read(AudioRegister reg) const override;
  float Sample() const override;
  void Tick() override;
  void Trigger() override;
  bool IsEnabled() const override;

  u8 ReadWave(u8 idx) const;
  void SetWave(u8 idx, u8 byte);

private:
  void TickLength() override;
  void TickEnvenlope() override;
  void TickSweep() override;

  u16 SetFrequency() const;
  void SetFrequency(u16 freq);

private:
  bool enable_channel_ {};
  u16 length_counter_ {};
  u16 timer_ {};
  u8 volume_ {};
  u8 wave_index_ {};
  u8 buffer_ {};
  u16 last_read_ {};

  std::array<u8, 16> wave_pattern_ram_;
  std::array<u8, 5> masks_;

  union {
    std::array<u8, 5> regs {};

    struct {
      struct {
        u8 : 7;
        u8 dac : 1;
      } nrx0;

      struct {
        u8 initial_length_timer : 8;
      } nrx1;

      struct {
        u8 : 5;
        u8 output_level : 2;
        u8 : 1;
      } nrx2;

      u8 nrx3;

      struct {
        u8 period : 3;
        u8 : 3;
        u8 length_enable : 1;
        u8 trigger : 1;
      } nrx4;
    };
  };
};
