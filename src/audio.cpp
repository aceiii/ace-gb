#include <spdlog/spdlog.h>

#include "audio.h"

constexpr int kClockSpeed = 4194304;
constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

Audio::Audio(Timer &timer, audio_config cfg): timer { timer }, config { cfg } {
  sample_buffer.reserve(cfg.buffer_size * cfg.num_channels);
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
      nr52.val = 0;
      nr50.val = 0;
      nr51.val = 0;
      ch1.reset();
      ch2.reset();
      ch3.reset();
      ch4.reset();
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

void Audio::on_tick() {
    ch1.tick();
    ch2.tick();
    ch3.tick();
    ch4.tick();

    auto bit = (timer.div() >> 4) & 0b1; // NOTE: bit 4 normal, 5 in double-speed mode
    if (bit == 0 && prev_bit == 1) {
      ch1.clock(frame_sequencer);
      ch2.clock(frame_sequencer);
      ch3.clock(frame_sequencer);
      ch4.clock(frame_sequencer);

      frame_sequencer = (frame_sequencer + 1) % 8;
    }

    prev_bit = bit;

    if (sample_timer) {
      sample_timer -= 1;
    }

    if (sample_timer == 0) {
      const auto [left, right] = sample();
      sample_buffer.push_back(left);
      sample_buffer.push_back(right);
      sample_timer = kClockSpeed / config.sample_rate;
    }
}

void Audio::get_samples(float *samples, size_t num_samples, size_t num_channels) {
  auto samples_to_copy = std::min(num_samples * num_channels, sample_buffer.size());
//  spdlog::info("get_samples: {}", samples_to_copy);
  std::copy_n(sample_buffer.begin(), samples_to_copy, samples);
  sample_buffer.erase(sample_buffer.begin(), sample_buffer.begin() + samples_to_copy);
}

std::tuple<float, float> Audio::sample() {
  float left = 0.0f;
  float right = 0.0f;

  if (nr52.audio) {
    auto s1 = ch1.sample();
    auto s2 = ch2.sample();
    auto s3 = ch3.sample();
    auto s4 = ch4.sample();

//    if (nr51.ch1_left) {
//      left += s1;
//    }
//    if (nr51.ch2_left) {
//      left += s2;
//    }
//    if (nr51.ch3_left) {
//      left += s3;
//    }
    if (nr51.ch4_left) {
      left += s4;
    }
//
//    if (nr51.ch1_right) {
//      right += s1;
//    }
//    if (nr51.ch2_right) {
//      right += s2;
//    }
//    if (nr51.ch3_right) {
//      right += s3;
//    }
    if (nr51.ch4_right) {
      right += s4;
    }

//    left /= 4.0f;
//    right /= 4.0f;
//    spdlog::info("sample: {}, {}", left, right);
  } else {
//    spdlog::info("off");
  }

  left = (left * static_cast<float>(nr50.left_volume + 1)) / 8.0f;
  right = (right * static_cast<float>(nr50.right_volume + 1)) / 8.0f;

//  spdlog::info("audio: {} {} {} {}", s1, s2, s3, s4);

  return std::make_tuple(left, right);
}
