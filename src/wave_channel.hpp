#pragma once

#include <array>

#include "audio_channel.hpp"

class WaveChannel : public AudioChannel {
public:
  explicit WaveChannel();

  void Reset() override;
  void PowerOff() override;
  void Write(AudioRegister reg, uint8_t value) override;
  uint8_t Read(AudioRegister reg) const override;
  float Sample() const override;
  void Tick() override;
  void Trigger() override;
  bool IsEnabled() const override;

  uint8_t ReadWave(uint8_t idx) const;
  void SetWave(uint8_t idx, uint8_t byte);

private:
  void TickLength() override;
  void TickEnvenlope() override;
  void TickSweep() override;

  uint16_t SetFrequency() const;
  void SetFrequency(uint16_t freq);

private:
  bool enable_channel_ {};
  uint16_t length_counter_ {};
  uint16_t timer_ {};
  uint8_t volume_ {};
  uint8_t wave_index_ {};
  uint8_t buffer_ {};
  uint16_t last_read_ {};

  std::array<uint8_t, 16> wave_pattern_ram_;
  std::array<uint8_t, 5> masks_;

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
