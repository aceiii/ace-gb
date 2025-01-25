#include <spdlog/spdlog.h>

#include "null_device.h"

[[nodiscard]] bool NullDevice::valid_for(uint16_t addr) const {
  return true;
}

void NullDevice::write8(uint16_t addr, uint8_t byte) {
  spdlog::warn("Write to 0x{:02x} = {:02x}", addr, byte);
}

[[nodiscard]] uint8_t NullDevice::read8(uint16_t addr) const {
  spdlog::warn("Read from 0x{:02x}", addr);
  return 0;
}

void NullDevice::reset() {
}
