#include <spdlog/spdlog.h>

#include "null_device.h"

[[nodiscard]] bool NullDevice::IsValidFor(u16 addr) const {
  return true;
}

void NullDevice::Write8(u16 addr, u8 byte) {
  if (auto it = overrides.find(addr); it != overrides.end() && it->second.writeable) {
    it->second.value = byte;
  }

  spdlog::warn("NullDevice: Write to 0x{:02x} = {:02x}", addr, byte);
}

[[nodiscard]] u8 NullDevice::Read8(u16 addr) const {
  if (auto it = overrides.find(addr); it != overrides.end()) {
    u8 val = it->second.value | it->second.mask;
    spdlog::info("NullDevice: Read from override:0x{:02x} -> {:02x}", addr, val);
    return it->second.value | it->second.mask;
  }

  spdlog::warn("NullDevice: Read from 0x{:02x}", addr);
  return 0xff;
}

void NullDevice::Reset() {
}

void NullDevice::add_override(u16 addr, u8 default_value, bool writable, u8 mask) {
  overrides[addr] = { default_value, writable, mask };
}
