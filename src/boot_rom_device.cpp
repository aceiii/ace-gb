#include <spdlog/spdlog.h>

#include "boot_rom_device.h"
#include "io.h"

bool BootRomDevice::valid_for(uint16_t addr) const {
  if (addr == std::to_underlying(IO::BOOT)) {
    return true;
  }

  if (addr < rom.size() && !disable) {
    return true;
  }

  return false;
}

void BootRomDevice::write8(uint16_t addr, uint8_t byte) {
  if (addr != std::to_underlying(IO::BOOT)) {
    return;
  }
  disable = byte;
}

[[nodiscard]] uint8_t BootRomDevice::read8(uint16_t addr) const {
  if (addr < rom.size()) {
    return rom[addr];
  }
  if (addr == std::to_underlying(IO::BOOT)) {
    return disable;
  }
  std::unreachable();
}

void BootRomDevice::reset() {
  disable = 0;
  rom.fill(0);
}

void BootRomDevice::load_bytes(const rom_buffer &bytes) {
  rom = bytes;
}
