#include "wram_device.hpp"
#include <cstddef>

namespace {

constexpr size_t kWramStart = 0xC000;
constexpr size_t kWramEnd = 0xDFFF;
constexpr size_t kEchoRamStart = 0xE000;
constexpr size_t kEchoRamEnd = 0xFDFF;

}

bool WramDevice::IsValidFor(u16 addr) const {
  return addr >= kWramStart && addr <= kEchoRamEnd;
}

void WramDevice::Write8(u16 addr, u8 byte) {
  auto wram_addr = addr - kWramStart;
  if (wram_addr < wram.size()) {
    wram[wram_addr] = byte;
  }
}

u8 WramDevice::Read8(u16 addr) const {
  auto wram_addr = addr - kWramStart;
  if (wram_addr < wram.size()) {
    return wram[wram_addr];
  }

  return wram[wram_addr - wram.size()];
}

void WramDevice::Reset() {
  wram.fill(0);
}
