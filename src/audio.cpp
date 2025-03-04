#include "audio.h"

constexpr int kWaveRamStart = std::to_underlying(IO::WAVE);
constexpr int kWaveRamEnd = kWaveRamStart + 15;

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  ram[addr - kAudioStart] = byte;
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
}

void Audio::get_samples(float *samples, size_t num_samples, size_t num_channels) {
  std::fill_n(samples, num_samples, 0);
}
