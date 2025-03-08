#include "wave_channel.h"

constexpr std::array<uint8_t, 16> kInitialWaveRam {{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }};

WaveChannel::WaveChannel(): wave_pattern_ram { kInitialWaveRam } {
}

void WaveChannel::reset() {
}

void WaveChannel::write(AudioRegister reg, uint8_t value) {
}

uint8_t WaveChannel::read(AudioRegister reg) const {
}

uint8_t WaveChannel::sample() const {
}

void WaveChannel::tick(uint8_t cycles) {
}

uint8_t WaveChannel::read_wave(uint8_t idx) const {
  return wave_pattern_ram[idx];
}

void WaveChannel::set_wave(uint8_t idx, uint8_t byte) {
  wave_pattern_ram[idx] = byte;
}

