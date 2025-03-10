#include <utility>

#include "square_channel.h"

using waveform = std::array<uint8_t, 8>;

std::array<waveform, 4> waveforms {{
  { 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 0, 0, 0, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 0 },
}};

constexpr std::array<uint8_t, 5> defaultMask(bool sweep) {
  if (sweep) {
    return { 0x80, 0x3f, 0x00, 0xff, 0xbf };
  }
  return { 0xff, 0x3f, 0x00, 0xff, 0xbf };
}

SquareChannel::SquareChannel(bool sweep): enable_sweep { sweep }, masks { defaultMask(sweep) } {
}

void SquareChannel::reset() {
  regs.fill(0);
}

void SquareChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  if (reg == AudioRegister::NRx4 && (value >> 7)) {
    trigger();
  }
}

uint8_t SquareChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

uint8_t SquareChannel::sample() const {
  return 0;
}

void SquareChannel::trigger() {
  enable_channel = nrx2.dac;
  period = nrx4.period;

  if (nrx0.step) {
    frequency >>= nrx0.step;
    if (frequency > 0x7ff) {
      enable_channel = false;
    }
  }
}

void SquareChannel::length_tick() {
}

void SquareChannel::envelope_tick() {
}

void SquareChannel::sweep_tick() {
}
