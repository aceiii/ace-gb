#include <algorithm>
#include <utility>
#include <spdlog/spdlog.h>

#include "wave_channel.hpp"

constexpr  auto kInitialLengthCounter = 256;

constexpr std::array<uint8_t, 16> kInitialWaveRam {{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }};

constexpr std::array<uint8_t, 5> kDefaultMasks {{ 0x7f, 0xff, 0x9f, 0xff, 0xbf }};

WaveChannel::WaveChannel(): wave_pattern_ram_ { kInitialWaveRam }, masks_ { kDefaultMasks } {
}

void WaveChannel::Reset() {
  enable_channel_ = false;
  length_counter_ = 0;
  timer_ = 0;
  volume_ = 0;
  wave_index_ = 0;
  buffer_ = 0;
  regs.fill(0);
  wave_pattern_ram_ = kInitialWaveRam;
}

void WaveChannel::PowerOff() {
  enable_channel_ = false;
  length_counter_ = 0;
  timer_ = 0;
  volume_ = 0;
  wave_index_ = 0;
  buffer_ = 0;
  regs.fill(0);
}

void WaveChannel::Write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx0:
      if (!nrx0.dac) {
        enable_channel_ = false;
      }
      break;
    case AudioRegister::NRx1:
      length_counter_ = kInitialLengthCounter - value;
      break;
    case AudioRegister::NRx2:
      volume_ = nrx2.output_level;
      break;
    case AudioRegister::NRx4:
      if (nrx4.trigger) {
        Trigger();
      }
      break;
    default: break;
  }
}

uint8_t WaveChannel::Read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks_[idx];
}

float WaveChannel::Sample() const {
  if (!enable_channel_ || !nrx0.dac) {
    return 0.0f;
  }

  auto shift = 4;
  if (volume_ > 0) {
    shift = volume_ - 1;
  }

  auto byte = buffer_ >> shift;
  return 1.0f - (static_cast<float>(byte) / 7.5f);
}

uint8_t WaveChannel::ReadWave(uint8_t idx) const {
  if (enable_channel_) {
    spdlog::info("wave channel read:{} = {:02x}, enable_channel:{}, last_read:{}", wave_index_, wave_pattern_ram_[wave_index_], enable_channel_, last_read_);
    if (last_read_) {
      return wave_pattern_ram_[wave_index_];
    }
    return 0xff;
  }
  return wave_pattern_ram_[idx];
}

void WaveChannel::SetWave(uint8_t idx, uint8_t byte) {
  if (enable_channel_) {
    if (last_read_) {
      wave_pattern_ram_[idx] = byte;
    }
    return;
  }
  wave_pattern_ram_[idx] = byte;
}

void WaveChannel::Tick() {
  if (!IsEnabled()) {
    return;
  }

  if (timer_) {
    timer_ -= 1;
  }

  if (last_read_) {
    last_read_ -= 1;
  }

  if (timer_ == 0) {
    wave_index_ = (wave_index_ + 1) % 32;
    buffer_ = (wave_pattern_ram_[wave_index_ / 2] >> ((1 - (wave_index_ % 2)) * 4)) & 0xf;
    timer_ = (2048 - SetFrequency()) * 2;
    last_read_ = 64;
  }
}

void WaveChannel::Trigger() {
  enable_channel_ = nrx0.dac;

  if (length_counter_ == 0) {
    length_counter_ = kInitialLengthCounter;
  }

  timer_ = (2048 - SetFrequency()) * 2;
  wave_index_ = 0;
  buffer_ = (wave_pattern_ram_[0] >> 4) & 0xf;
}

bool WaveChannel::IsEnabled() const {
  return enable_channel_;
}

void WaveChannel::TickLength() {
  if (!nrx4.length_enable) {
    return;
  }

  if (length_counter_) {
    length_counter_ -= 1;
  }

  if (length_counter_ == 0) {
    enable_channel_ = false;
  }
}

void WaveChannel::TickEnvenlope() {
}

void WaveChannel::TickSweep() {
}

uint16_t WaveChannel::SetFrequency() const {
  return nrx3 | (nrx4.period << 8);
}

void WaveChannel::SetFrequency(uint16_t freq) {
  nrx3 = freq & 0xff;
  nrx4.period = (freq >> 8) & 0b111;
}
