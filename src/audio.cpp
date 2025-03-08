#include "audio.h"

constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

Audio::Audio(Timer &timer): timer { timer } {
}

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    ch3.set_wave(addr - kWaveRamStart, byte);
  } else if (addr == std::to_underlying(IO::NR52)) {
    uint8_t enable_audio = byte >> 7;
    nr52.audio = enable_audio;
    if (!nr52.audio) {
      nr50.val = 0;
      nr51.val = 0;
      ch1.reset();
      ch2.reset();
      ch3.reset();
      ch4.reset();
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
    return nr52.val | 0b01110000;
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
  nr50.val = 0;
  nr51.val = 0;
  nr52.val = 0;
  ch1.reset();
  ch2.reset();
  ch3.reset();
  ch4.reset();
}

void Audio::on_tick() {
  auto div = timer.div();
  if (div == 0) {
    frame_sequencer = (frame_sequencer + 1) % 8;
  }

  auto [left, right] = sample();
}

void Audio::get_samples(float *samples, size_t num_samples, size_t num_channels) {
  std::fill_n(samples, num_samples, 0);
}

std::tuple<float, float> Audio::sample() {

}
