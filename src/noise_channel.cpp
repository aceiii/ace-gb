#include <utility>

#include "noise_channel.h"

constexpr std::array<uint8_t, 5> kDefaultMasks {{ 0xff, 0xff, 0x00, 0x00, 0xbf }};

NoiseChannel::NoiseChannel(): masks { kDefaultMasks } {
}

void NoiseChannel::reset() {
  regs.fill(0);
}

void NoiseChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  if (reg == AudioRegister::NRx4 && (value >> 7)) {
    trigger();
  }
}

uint8_t NoiseChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

uint8_t NoiseChannel::sample() const {
  return 0;
}

void NoiseChannel::clock(uint8_t sequence) {
}

void NoiseChannel::trigger() {
  enable_channel = nrx2.dac;
}
