#pragma once

#include "audio_channel.h"

class NoiseChannel : public AudioChannel {
public:
  explicit NoiseChannel();

  void reset() override;
  void write(AudioRegister reg, uint8_t value) override;
  uint8_t read(AudioRegister reg) const override;
  uint8_t sample() const override;
  void tick(uint8_t cycles) override;
private:

};
