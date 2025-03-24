#include <algorithm>
#include <utility>

#include "wave_channel.h"

constexpr  auto kInitialLengthCounter = 256;

constexpr std::array<uint8_t, 16> kInitialWaveRam {{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }};

constexpr std::array<uint8_t, 5> kDefaultMasks {{ 0x7f, 0xff, 0x9f, 0xff, 0xbf }};

WaveChannel::WaveChannel(): wave_pattern_ram { kInitialWaveRam }, masks { kDefaultMasks } {
}

void WaveChannel::reset() {
  enable_channel = false;
  length_counter = 0;
  timer = 0;
  volume = 0;
  wave_index = 0;
  buffer = 0;
  regs.fill(0);
  wave_pattern_ram = kInitialWaveRam;
}

void WaveChannel::poweroff() {
  enable_channel = false;
  length_counter = 0;
  timer = 0;
  volume = 0;
  wave_index = 0;
  buffer = 0;
  regs.fill(0);
}

void WaveChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx0:
      if (!nrx0.dac) {
        enable_channel = false;
      }
      break;
    case AudioRegister::NRx1:
      length_counter = kInitialLengthCounter - value;
      break;
    case AudioRegister::NRx2:
      volume = nrx2.output_level;
      break;
    case AudioRegister::NRx4:
      if (nrx4.trigger) {
        trigger();
      }
      break;
    default: break;
  }
}

uint8_t WaveChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

float WaveChannel::sample() const {
  if (!enable_channel || !nrx0.dac) {
    return 0.0f;
  }

  auto shift = 4;
  if (volume > 0) {
    shift = volume - 1;
  }

  auto byte = buffer >> shift;
  return (static_cast<float>(byte) / 7.5f) - 1.0f;
}

uint8_t WaveChannel::read_wave(uint8_t idx) const {
  return wave_pattern_ram[idx];
}

void WaveChannel::set_wave(uint8_t idx, uint8_t byte) {
  wave_pattern_ram[idx] = byte;
}

void WaveChannel::tick() {
  if (timer) {
    timer -= 1;
  }

  if (timer == 0) {
    wave_index = (wave_index + 1) % 32;
    buffer = (wave_pattern_ram[wave_index / 2] >> ((1 - (wave_index % 2)) * 4)) & 0xf;
    timer = (2048 - frequency()) * 2;
  }
}

void WaveChannel::trigger() {
  enable_channel = nrx0.dac;

  if (length_counter == 0) {
    length_counter = kInitialLengthCounter;
  }

  timer = (2048 - frequency()) * 2;
  wave_index = 0;
}

bool WaveChannel::enabled() const {
  return enable_channel;
}

void WaveChannel::length_tick() {
  if (!nrx4.length_enable) {
    return;
  }

  if (length_counter) {
    length_counter -= 1;
  }

  if (length_counter == 0) {
    enable_channel = false;
  }
}

void WaveChannel::envelope_tick() {
}

void WaveChannel::sweep_tick() {
}

uint16_t WaveChannel::frequency() const {
  return nrx3 | (nrx4.period << 8);
}

void WaveChannel::set_frequency(uint16_t freq) {
  nrx3 = freq & 0xff;
  nrx4.period = (freq >> 8) & 0b111;
}
