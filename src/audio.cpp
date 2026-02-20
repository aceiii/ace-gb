#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

#include "audio.hpp"

constexpr int kClockSpeed = 4194304;
constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

Audio::Audio(Timer& timer, audio_config cfg) : timer_{ timer }, config_{ cfg } {
}

bool Audio::IsValidFor(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::Write8(uint16_t addr, uint8_t byte) {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    ch3_.SetWave(addr - kWaveRamStart, byte);
  } else if (addr == std::to_underlying(IO::NR52)) {
    uint8_t enable_audio = byte >> 7;
    nr52_.val = byte;
    if (!enable_audio) {
      PowerOff();
    } else {
      spdlog::info("Enable audio!");
    }
  } else if (nr52_.audio) {
    if (addr >= std::to_underlying(IO::NR10) && addr <= std::to_underlying(IO::NR14)) {
      ch1_.Write(AudioRegister { addr - std::to_underlying(IO::NR10) }, byte);
    } else if (addr >= std::to_underlying(IO::NR20) && addr <= std::to_underlying(IO::NR24)) {
      ch2_.Write(AudioRegister { addr - std::to_underlying(IO::NR20) }, byte);
    } else if (addr >= std::to_underlying(IO::NR30) && addr <= std::to_underlying(IO::NR34)) {
      ch3_.Write(AudioRegister { addr - std::to_underlying(IO::NR30) }, byte);
    } else if (addr >= std::to_underlying(IO::NR40) && addr <= std::to_underlying(IO::NR44)) {
      ch4_.Write(AudioRegister { addr - std::to_underlying(IO::NR40) }, byte);
    } else if (addr == std::to_underlying(IO::NR50)) {
      nr50_.val = byte;
    } else if (addr == std::to_underlying(IO::NR51)) {
      nr51_.val = byte;
    } else if (addr == std::to_underlying(IO::NR52)) {
      nr52_.val = byte;
    }
  } else {
    if (addr == std::to_underlying(IO::NR41)) {
      ch4_.Write(AudioRegister::NRx1, byte);
    }
  }
}

uint8_t Audio::Read8(uint16_t addr) const {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    return ch3_.ReadWave(addr - kWaveRamStart);
  }

  if (addr == std::to_underlying(IO::NR50)) {
    return nr50_.val;
  }
  if (addr == std::to_underlying(IO::NR51)) {
    return nr51_.val;
  }
  if (addr == std::to_underlying(IO::NR52)) {
    auto hi = ((nr52_.val | 0b01110000) & 0b11110000);
    uint8_t lo = (ch1_.IsEnabled() & 0b1) | ((ch2_.IsEnabled() & 0b1) << 1) | ((ch3_.IsEnabled() & 0b1) << 2) | ((ch4_.IsEnabled() & 0b1) << 3);
    spdlog::info("NR52: {:08b}, div_timer:{}", (hi | lo), timer_.div());
    return hi | lo;
  }

  if (addr >= std::to_underlying(IO::NR10) && addr <= std::to_underlying(IO::NR14)) {
    return ch1_.Read(AudioRegister { addr - std::to_underlying(IO::NR10) });
  }

  if (addr >= std::to_underlying(IO::NR21) && addr <= std::to_underlying(IO::NR24)) {
    return ch2_.Read(AudioRegister { addr - std::to_underlying(IO::NR20) });
  }

  if (addr >= std::to_underlying(IO::NR30) && addr <= std::to_underlying(IO::NR34)) {
    return ch3_.Read(AudioRegister { addr - std::to_underlying(IO::NR30) });
  }

  if (addr >= std::to_underlying(IO::NR41) && addr <= std::to_underlying(IO::NR44)) {
    return ch4_.Read(AudioRegister { addr - std::to_underlying(IO::NR40) });
  }

  return 0xff;
}

void Audio::Reset() {
  sample_timer_ = 0;
  nr50_.val = 0;
  nr51_.val = 0;
  nr52_.val = 0;
  ch1_.Reset();
  ch2_.Reset();
  ch3_.Reset();
  ch4_.Reset();
}

void Audio::PowerOff() {
  sample_timer_ = 0;
  nr50_.val = 0;
  nr51_.val = 0;
  nr52_.val = 0;
  ch1_.PowerOff();
  ch2_.PowerOff();
  ch3_.PowerOff();
  ch4_.PowerOff();
}

void Audio::OnTick() {
  ZoneScoped;

  ch1_.Tick();
  ch2_.Tick();
  ch3_.Tick();
  ch4_.Tick();

  frame_sequencer_counter_ += 1;
  while (frame_sequencer_counter_ >= 8192) {
    frame_sequencer_counter_ -= 8192;
    ch1_.Clock(frame_sequencer_);
    ch2_.Clock(frame_sequencer_);
    ch3_.Clock(frame_sequencer_);
    ch4_.Clock(frame_sequencer_);
    frame_sequencer_ = (frame_sequencer_ + 1) % 8;
  }

  if (sample_timer_) {
    sample_timer_ -= 1;
  }

  if (sample_timer_ == 0) {
    const auto [left, right] = Sample();
    sample_buffer_[buffer_write_idx_] = left;
    sample_buffer_[buffer_write_idx_ + 1] = right;
    buffer_write_idx_ = (buffer_write_idx_ + 2) % sample_buffer_.size();
    sample_timer_ += kClockSpeed / config_.sample_rate;
  }
}

void Audio::GetSamples(std::vector<float>& out_buffer) {
  for (auto it = out_buffer.begin(); it != out_buffer.end(); ++it) {
    if (buffer_read_idx_ == buffer_write_idx_) {
      *it = 0;
      continue;
    }
    *it = sample_buffer_[buffer_read_idx_];
    buffer_read_idx_ = (buffer_read_idx_ + 1) % sample_buffer_.size();
  }
}

std::tuple<float, float> Audio::Sample() const {
  float left = 0.0f;
  float right = 0.0f;

  if (nr52_.audio && enable_channel_[4]) {
    auto s1 = ch1_.Sample();
    auto s2 = ch2_.Sample();
    auto s3 = ch3_.Sample();
    auto s4 = ch4_.Sample();

    if (nr51_.ch1_left && enable_channel_[0]) {
      left += s1;
    }
    if (nr51_.ch2_left && enable_channel_[1]) {
      left += s2;
    }
    if (nr51_.ch3_left && enable_channel_[2]) {
      left += s3;
    }
    if (nr51_.ch4_left && enable_channel_[3]) {
      left += s4;
    }

    if (nr51_.ch1_right && enable_channel_[0]) {
      right += s1;
    }
    if (nr51_.ch2_right && enable_channel_[1]) {
      right += s2;
    }
    if (nr51_.ch3_right && enable_channel_[2]) {
      right += s3;
    }
    if (nr51_.ch4_right && enable_channel_[3]) {
      right += s4;
    }

    left /= 4.0f;
    right /= 4.0f;
  }

  left = left * nr50_.left_volume / 7.0f;
  right = right * nr50_.right_volume / 7.0f;

  return std::make_tuple(left, right);
}

bool Audio::IsChannelEnabled(AudioChannelID channel) const {
  return enable_channel_[std::to_underlying(channel)];
}

void Audio::ToggleChannel(AudioChannelID channel, bool enable) {
  enable_channel_[std::to_underlying(channel)] = enable;
}
