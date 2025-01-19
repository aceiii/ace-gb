#include "wram_device.h"

namespace {

constexpr size_t kWramStart = 0xC000;
constexpr size_t kWramEnd = 0xDFFF;
constexpr size_t kEchoRamStart = 0xE000;
constexpr size_t kEchoRamEnd = 0xFDFF;

}

bool WramDevice::valid_for(uint16_t addr) const {
  return addr >= kWramStart && addr <= kEchoRamEnd;
}

void WramDevice::write8(uint16_t addr, uint8_t byte) {
  auto wram_addr = addr - kWramStart;
  if (wram_addr < wram.size()) {
    wram[wram_addr] = byte;
  }
}

uint8_t WramDevice::read8(uint16_t addr) const {
  auto wram_addr = addr - kWramStart;
  if (wram_addr < wram.size()) {
    return wram[wram_addr];
  }

  return wram[wram_addr - wram.size()];
}

void WramDevice::reset() {
  wram.fill(0);
}
