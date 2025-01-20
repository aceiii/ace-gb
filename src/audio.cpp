#include "audio.h"

bool Audio::valid_for(uint16_t addr) const {
  return addr >= kAudioStart && addr <= kAudioEnd;
}

void Audio::write8(uint16_t addr, uint8_t byte) {
  ram[addr - kAudioStart] = byte;
}

uint8_t Audio::read8(uint16_t addr) const {
  return ram[addr - kAudioStart];
}

void Audio::reset() {
  ram.fill(0);
}
