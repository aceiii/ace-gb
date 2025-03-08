#pragma once

#include <array>

#include "audio_channel.h"

class WaveChannel : public AudioChannel {
public:
  explicit WaveChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  uint8_t sample() const override;
  void tick(uint8_t cycles) override;

  uint8_t read_wave(uint8_t idx) const;
  void set_wave(uint8_t idx, uint8_t byte);

private:
  std::array<uint8_t, 16> wave_pattern_ram;
};
