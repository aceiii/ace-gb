#include <spdlog/spdlog.h>

#include "audio.h"

constexpr int kClockSpeed = 4194304;
constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

Audio::Audio(Timer &timer, audio_config cfg) : timer{ timer }, config{ cfg } {
}

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    ch3.set_wave(addr - kWaveRamStart, byte);
  } else if (addr == std::to_underlying(IO::NR52)) {
    uint8_t enable_audio = byte >> 7;
    nr52.val = byte;
    if (!enable_audio) {
      poweroff();
    } else {
      spdlog::info("Enable audio!");
    }
  } else if (nr52.audio) {
    if (addr >= std::to_underlying(IO::NR10) && addr <= std::to_underlying(IO::NR14)) {
      ch1.write(AudioRegister { addr - std::to_underlying(IO::NR10) }, byte);
    } else if (addr >= std::to_underlying(IO::NR20) && addr <= std::to_underlying(IO::NR24)) {
      ch2.write(AudioRegister { addr - std::to_underlying(IO::NR20) }, byte);
    } else if (addr >= std::to_underlying(IO::NR30) && addr <= std::to_underlying(IO::NR34)) {
      ch3.write(AudioRegister { addr - std::to_underlying(IO::NR30) }, byte);
    } else if (addr >= std::to_underlying(IO::NR40) && addr <= std::to_underlying(IO::NR44)) {
      ch4.write(AudioRegister { addr - std::to_underlying(IO::NR40) }, byte);
    } else if (addr == std::to_underlying(IO::NR50)) {
      nr50.val = byte;
    } else if (addr == std::to_underlying(IO::NR51)) {
      nr51.val = byte;
    } else if (addr == std::to_underlying(IO::NR52)) {
      nr52.val = byte;
    }
  } else {
    if (addr == std::to_underlying(IO::NR41)) {
      ch4.write(AudioRegister::NRx1, byte);
    }
  }
}

uint8_t Audio::read8(uint16_t addr) const {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    return ch3.read_wave(addr - kWaveRamStart);
  }

  if (addr == std::to_underlying(IO::NR50)) {
    return nr50.val;
  }
  if (addr == std::to_underlying(IO::NR51)) {
    return nr51.val;
  }
  if (addr == std::to_underlying(IO::NR52)) {
    auto hi = ((nr52.val | 0b01110000) & 0b11110000);
    uint8_t lo = (ch1.enabled() & 0b1) | ((ch2.enabled() & 0b1) << 1) | ((ch3.enabled() & 0b1) << 2) | ((ch4.enabled() & 0b1) << 3);
    spdlog::info("NR52: {:08b}, div_timer:{}", (hi | lo), timer.div());
    return hi | lo;
  }

  if (addr >= std::to_underlying(IO::NR10) && addr <= std::to_underlying(IO::NR14)) {
    return ch1.read(AudioRegister { addr - std::to_underlying(IO::NR10) });
  }

  if (addr >= std::to_underlying(IO::NR21) && addr <= std::to_underlying(IO::NR24)) {
    return ch2.read(AudioRegister { addr - std::to_underlying(IO::NR20) });
  }

  if (addr >= std::to_underlying(IO::NR30) && addr <= std::to_underlying(IO::NR34)) {
    return ch3.read(AudioRegister { addr - std::to_underlying(IO::NR30) });
  }

  if (addr >= std::to_underlying(IO::NR41) && addr <= std::to_underlying(IO::NR44)) {
    return ch4.read(AudioRegister { addr - std::to_underlying(IO::NR40) });
  }

  return 0xff;
}

void Audio::reset() {
  sample_timer = 0;
  nr50.val = 0;
  nr51.val = 0;
  nr52.val = 0;
  ch1.reset();
  ch2.reset();
  ch3.reset();
  ch4.reset();
}

void Audio::poweroff() {
  sample_timer = 0;
  nr50.val = 0;
  nr51.val = 0;
  nr52.val = 0;
  ch1.poweroff();
  ch2.poweroff();
  ch3.poweroff();
  ch4.poweroff();
}

void Audio::OnTick() {
  ch1.tick();
  ch2.tick();
  ch3.tick();
  ch4.tick();

  frame_sequencer_counter += 1;
  while (frame_sequencer_counter >= 8192) {
    frame_sequencer_counter -= 8192;
    ch1.clock(frame_sequencer);
    ch2.clock(frame_sequencer);
    ch3.clock(frame_sequencer);
    ch4.clock(frame_sequencer);
    frame_sequencer = (frame_sequencer + 1) % 8;
  }

  if (sample_timer) {
    sample_timer -= 1;
  }

  if (sample_timer == 0) {
    const auto [left, right] = sample();
    sample_buffer[buffer_write_idx] = left;
    sample_buffer[buffer_write_idx + 1] = right;
    buffer_write_idx = (buffer_write_idx + 2) % sample_buffer.size();
    sample_timer += kClockSpeed / config.sample_rate;
  }
}

void Audio::get_samples(std::vector<float> &out_buffer) {
  for (auto it = out_buffer.begin(); it != out_buffer.end(); ++it) {
    if (buffer_read_idx == buffer_write_idx) {
      *it = 0;
      continue;
    }
    *it = sample_buffer[buffer_read_idx];
    buffer_read_idx = (buffer_read_idx + 1) % sample_buffer.size();
  }
}

std::tuple<float, float> Audio::sample() const {
  float left = 0.0f;
  float right = 0.0f;

  if (nr52.audio && enable_channel[4]) {
    auto s1 = ch1.sample();
    auto s2 = ch2.sample();
    auto s3 = ch3.sample();
    auto s4 = ch4.sample();

    if (nr51.ch1_left && enable_channel[0]) {
      left += s1;
    }
    if (nr51.ch2_left && enable_channel[1]) {
      left += s2;
    }
    if (nr51.ch3_left && enable_channel[2]) {
      left += s3;
    }
    if (nr51.ch4_left && enable_channel[3]) {
      left += s4;
    }

    if (nr51.ch1_right && enable_channel[0]) {
      right += s1;
    }
    if (nr51.ch2_right && enable_channel[1]) {
      right += s2;
    }
    if (nr51.ch3_right && enable_channel[2]) {
      right += s3;
    }
    if (nr51.ch4_right && enable_channel[3]) {
      right += s4;
    }

    left /= 4.0f;
    right /= 4.0f;
  }

  left = left * nr50.left_volume / 7.0f;
  right = right * nr50.right_volume / 7.0f;

  return std::make_tuple(left, right);
}

bool Audio::channel_enabled(AudioChannelID channel) const {
  return enable_channel[std::to_underlying(channel)];
}

void Audio::toggle_channel(AudioChannelID channel, bool enable) {
  enable_channel[std::to_underlying(channel)] = enable;
}
