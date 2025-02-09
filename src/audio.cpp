#include "audio.h"

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  ram[addr - kAudioStart] = byte;
}

uint8_t Audio::read8(uint16_t addr) const {
  auto index = addr - kAudioStart;
  switch (addr) {
    case std::to_underlying(IO::NR10):
      return ram[index] | 0b10000000;
    case std::to_underlying(IO::NR11):
      return ram[index];
    case std::to_underlying(IO::NR12):
      return ram[index];
    case std::to_underlying(IO::NR13):
      return ram[index];
    case std::to_underlying(IO::NR14):
      return ram[index];
    case std::to_underlying(IO::NR21):
      return ram[index];
    case std::to_underlying(IO::NR22):
      return ram[index];
    case std::to_underlying(IO::NR23):
      return ram[index];
    case std::to_underlying(IO::NR24):
      return ram[index];
    case std::to_underlying(IO::NR30):
      return ram[index] | 0b01111111;
    case std::to_underlying(IO::NR31):
      return ram[index];
    case std::to_underlying(IO::NR32):
      return ram[index] | 0b10011111;
    case std::to_underlying(IO::NR33):
      return ram[index];
    case std::to_underlying(IO::NR34):
      return ram[index] | 0b00111000;
    case std::to_underlying(IO::NR41):
      return ram[index] | 0b11000000;
    case std::to_underlying(IO::NR42):
      return ram[index];
    case std::to_underlying(IO::NR43):
      return ram[index];
    case std::to_underlying(IO::NR44):
      return ram[index] | 0b00111111;
    case std::to_underlying(IO::NR50):
      return ram[index];
    case std::to_underlying(IO::NR51):
      return ram[index];
    case std::to_underlying(IO::NR52):
      return ram[index] | 0b01110000;
    default: return 0xff;
  }
}

void Audio::reset() {
  ram.fill(0);
}
