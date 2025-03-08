#include <utility>

#include "wave_channel.h"

constexpr std::array<uint8_t, 16> kInitialWaveRam {{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }};

constexpr std::array<uint8_t, 5> kDefaultMasks {{ 0x7f, 0xff, 0x9f, 0xff, 0xbf }};

WaveChannel::WaveChannel(): wave_pattern_ram { kInitialWaveRam }, masks { kDefaultMasks } {
}

void WaveChannel::reset() {
  regs.fill(0);
}

void WaveChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;
}

uint8_t WaveChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

uint8_t WaveChannel::sample() const {
  return 0;
}

void WaveChannel::clock() {
}

uint8_t WaveChannel::read_wave(uint8_t idx) const {
  return wave_pattern_ram[idx];
}

void WaveChannel::set_wave(uint8_t idx, uint8_t byte) {
  wave_pattern_ram[idx] = byte;
}

void WaveChannel::trigger() {
}
