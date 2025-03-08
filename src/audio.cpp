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
//
//    if (addr >= std::to_underlying(IO::NR10))
//
//
//      ram[addr - kAudioStart] = byte;
//
//    switch (addr) {
//      case std::to_underlying(IO::NR14):ch1.enable = true;
//        if (ch1.length_timer >= 64) {
//          ch1.length_timer = nr11.initial_length_timer;
//        }
//        ch1.period = nr13 | (nr14.period << 8);
//        ch1.envelope_timer = 0;
//        ch1.volume = nr12.initial_volume;
//        break;
//      case std::to_underlying(IO::NR24):
//      case std::to_underlying(IO::NR34):
//      case std::to_underlying(IO::NR44):break;
//      case std::to_underlying(IO::NR12):ch1.dac = nr12.dac != 0;
//        break;
//    }
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

  if (addr >= std::to_underlying(IO::NR20) && addr <= std::to_underlying(IO::NR24)) {
    return ch2.read(AudioRegister { addr - std::to_underlying(IO::NR10) });
  }

  if (addr >= std::to_underlying(IO::NR30) && addr <= std::to_underlying(IO::NR34)) {
    return ch3.read(AudioRegister { addr - std::to_underlying(IO::NR10) });
  }

  if (addr >= std::to_underlying(IO::NR40) && addr <= std::to_underlying(IO::NR44)) {
    return ch4.read(AudioRegister { addr - std::to_underlying(IO::NR10) });
  }

  return 0xff;

//  auto index = addr - kAudioStart;
//  switch (addr) {
//    case std::to_underlying(IO::NR10):
//      return ram[index] | 0b10000000;
//    case std::to_underlying(IO::NR30):
//      return ram[index] | 0b01111111;
//    case std::to_underlying(IO::NR32):
//      return ram[index] | 0b10011111;
//    case std::to_underlying(IO::NR34):
//      return ram[index] | 0b00111000;
//    case std::to_underlying(IO::NR41):
//      return ram[index] | 0b11000000;
//    case std::to_underlying(IO::NR44):
//      return ram[index] | 0b00111111;
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
    div_apu = (div_apu + 1) % 8;
  }
}

void Audio::get_samples(float *samples, size_t num_samples, size_t num_channels) {
  std::fill_n(samples, num_samples, 0);
}
