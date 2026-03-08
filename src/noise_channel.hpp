#pragma once

#include <array>

#include "audio_channel.hpp"

class NoiseChannel : public AudioChannel {
public:
  explicit NoiseChannel();

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

private:
  bool enable_channel {};
  u16 length_counter {};
  u16 envelope_timer {};
  u16 timer {};
  u8 volume {};
  union {
    u16 bytes {};
    struct {
      u16 val: 15;
      u16 temp: 1;
    };
    struct {
      u8 bit0: 1;
      u8 bit1: 1;
      u8 : 5;
      u8 mid: 1;
      u8 : 8;
    };
  } lfsr;

  std::array<u8, 5> masks;

  union {
    std::array<u8, 5> regs {};

    struct {
      u8 nrx0; // unused

      struct {
        u8 initial_length_timer: 6;
        u8 : 2;
      } nrx1;

      union {
        struct {
          u8 : 3;
          u8 dac: 5;
        };
        struct {
          u8 sweep_pace: 3;
          u8 env_dir: 1;
          u8 initial_volume: 4;
        };
      } nrx2;

      struct {
        u8 clock_divider : 3;
        u8 lfsr_width : 1;
        u8 clock_shift : 4;
      } nrx3;

      struct {
        u8 : 6;
        u8 length_enable: 1;
        u8 trigger: 1;
      } nrx4;
    };
  };
};
