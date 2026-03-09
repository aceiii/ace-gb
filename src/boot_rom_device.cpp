#include <spdlog/spdlog.h>

#include "boot_rom_device.hpp"
#include "io.hpp"

namespace {
  constexpr int kMinBootRomSize = 256;
};

bool BootRomDevice::IsValidFor(u16 addr) const {
  return addr == std::to_underlying(IO::BOOT) || (!disable_ && addr < rom_.size());
}

void BootRomDevice::Write8(u16 addr, u8 byte) {
  if (addr != std::to_underlying(IO::BOOT)) {
    return;
  }
  disable_ = byte;
}

[[nodiscard]] u8 BootRomDevice::Read8(u16 addr) const {
  if (addr < rom_.size()) {
    return rom_[addr];
  }

  return 0xff;
}

void BootRomDevice::Reset() {
  disable_ = 0;
  rom_.clear();
}

void BootRomDevice::LoadBytes(std::span<const u8> bytes) {
  rom_.assign(bytes.begin(), bytes.end());
}

void BootRomDevice::SetDisable(u8 byte) {
  disable_ = byte;
}
