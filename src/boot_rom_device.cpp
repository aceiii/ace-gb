#include <spdlog/spdlog.h>

#include "boot_rom_device.h"
#include "io.h"

bool BootRomDevice::valid_for(uint16_t addr) const {
  if (addr == std::to_underlying(IO::BOOT)) {
    return true;
  }

  if (addr < kBootRomSize && !disable) {
    return true;
  }

  return false;
}

void BootRomDevice::write8(uint16_t addr, uint8_t byte) {
  if (addr == std::to_underlying(IO::BOOT)) {
    spdlog::debug("Writing to BOOT flag: {}", byte);
    disable = byte;
    return;
  }
}

[[nodiscard]] uint8_t BootRomDevice::read8(uint16_t addr) const {
  if (addr < kBootRomSize) {
    return rom[addr];
  }
  if (addr == std::to_underlying(IO::BOOT)) {
    return disable;
  }
  std::unreachable();
}

void BootRomDevice::reset() {
  disable = 0;
}

void BootRomDevice::load_bytes(const std::vector<uint8_t> &bytes) {
  auto size = std::min(bytes.size(), rom.size());
  std::copy_n(bytes.begin(), size, rom.begin());
}
