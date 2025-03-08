#include <utility>

#include "square_channel.h"


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
}

uint8_t SquareChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

uint8_t SquareChannel::sample() const {
  return 0;
}

void SquareChannel::clock() {
}

void SquareChannel::trigger() {
}
