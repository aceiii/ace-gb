#include "audio.h"

constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

Audio::Audio(Timer &timer):timer{timer} {
}

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    wave_pattern_ram[addr - kWaveRamStart] = byte;
  } else if (addr == std::to_underlying(IO::NR52)) {
    uint8_t enable_audio = byte >> 7;
    nr52.audio = enable_audio;
    if (!nr52.audio) {
      std::fill_n(ram.data(), std::to_underlying(IO::NR51) - std::to_underlying(IO::NR10) + 1, 0);
    }
  } else if (nr52.audio) {
    ram[addr - kAudioStart] = byte;

    switch (addr) {
      case std::to_underlying(IO::NR14):
        ch1.enable = true;
        if (ch1.length_timer >= 64) {
          ch1.length_timer = nr11.initial_length_timer;
        }
        ch1.period = nr13 | (nr14.period << 8);
        ch1.envelope_timer = 0;
        ch1.volume = nr12.initial_volume;
        break;
      case std::to_underlying(IO::NR24):
      case std::to_underlying(IO::NR34):
      case std::to_underlying(IO::NR44):
        break;
      case std::to_underlying(IO::NR12):
        ch1.dac = nr12.dac != 0;
      break;
    }
  }
}

uint8_t Audio::read8(uint16_t addr) const {
  if (addr >= kWaveRamStart && addr <= kWaveRamEnd) {
    return wave_pattern_ram[addr - kWaveRamStart];
  }

  auto index = addr - kAudioStart;
  switch (addr) {
    case std::to_underlying(IO::NR10):
      return ram[index] | 0b10000000;
    case std::to_underlying(IO::NR30):
      return ram[index] | 0b01111111;
    case std::to_underlying(IO::NR32):
      return ram[index] | 0b10011111;
    case std::to_underlying(IO::NR34):
      return ram[index] | 0b00111000;
    case std::to_underlying(IO::NR41):
      return ram[index] | 0b11000000;
    case std::to_underlying(IO::NR44):
      return ram[index] | 0b00111111;
    case std::to_underlying(IO::NR52):
      return ram[index] | 0b01110000;
    case std::to_underlying(IO::NR11):
    case std::to_underlying(IO::NR12):
    case std::to_underlying(IO::NR13):
    case std::to_underlying(IO::NR14):
    case std::to_underlying(IO::NR21):
    case std::to_underlying(IO::NR22):
    case std::to_underlying(IO::NR23):
    case std::to_underlying(IO::NR24):
    case std::to_underlying(IO::NR31):
    case std::to_underlying(IO::NR33):
    case std::to_underlying(IO::NR42):
    case std::to_underlying(IO::NR43):
    case std::to_underlying(IO::NR50):
    case std::to_underlying(IO::NR51):
      return ram[index];
    default: return 0xff;
  }
}

void Audio::reset() {
  ram.fill(0);
}

void Audio::on_tick() {
  auto div = timer.div();
  if (div == 0) {
    div_apu += 1;
  }

  if (div % 8 == 0) {

  }
  if (div % 4 == 0) {

  }
  if (div % 2 == 0) {
    if (ch1.enable) {
      ch1.length_timer += 1;
      if (ch1.length_timer >= 64) {
        ch1.length_timer = nr11.initial_length_timer;
        ch1.enable = false;
      }
    }
  }
}

void Audio::get_samples(float *samples, size_t num_samples, size_t num_channels) {
  std::fill_n(samples, num_samples, 0);
}
