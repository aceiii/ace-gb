#include <spdlog/spdlog.h>

#include "boot_rom_device.hpp"
#include "io.h"

bool BootRomDevice::IsValidFor(uint16_t addr) const {
  return addr == std::to_underlying(IO::BOOT) || (!disable_ && addr < rom_.size());
}

void BootRomDevice::Write8(uint16_t addr, uint8_t byte) {
  if (addr != std::to_underlying(IO::BOOT)) {
    return;
  }
  disable_ = byte;
}

[[nodiscard]] uint8_t BootRomDevice::Read8(uint16_t addr) const {
  if (addr < rom_.size()) {
    return rom_[addr];
  }

  return 0xff;
}

void BootRomDevice::Reset() {
  disable_ = 0;
  rom_.fill(0);
}

void BootRomDevice::LoadBytes(const RomBuffer &bytes) {
  rom_ = bytes;
}

void BootRomDevice::SetDisable(uint8_t byte) {
  disable_ = byte;
}
