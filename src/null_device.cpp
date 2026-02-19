#include <spdlog/spdlog.h>

#include "null_device.h"

[[nodiscard]] bool NullDevice::IsValidFor(uint16_t addr) const {
  return true;
}

void NullDevice::Write8(uint16_t addr, uint8_t byte) {
  if (auto it = overrides.find(addr); it != overrides.end() && it->second.writeable) {
    it->second.value = byte;
  }

  spdlog::warn("NullDevice: Write to 0x{:02x} = {:02x}", addr, byte);
}

[[nodiscard]] uint8_t NullDevice::Read8(uint16_t addr) const {
  if (auto it = overrides.find(addr); it != overrides.end()) {
    uint8_t val = it->second.value | it->second.mask;
    spdlog::info("NullDevice: Read from override:0x{:02x} -> {:02x}", addr, val);
    return it->second.value | it->second.mask;
  }

  spdlog::warn("NullDevice: Read from 0x{:02x}", addr);
  return 0xff;
}

void NullDevice::Reset() {
}

void NullDevice::add_override(uint16_t addr, uint8_t default_value, bool writable, uint8_t mask) {
  overrides[addr] = { default_value, writable, mask };
}
