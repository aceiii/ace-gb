#include <spdlog/spdlog.h>

#include "mmu.h"

void Mmu::clear_devices() {
  devices.clear();
}

void Mmu::add_device(mmu_device_ptr device) {
  devices.emplace_back(device);
}

void Mmu::write8(uint16_t addr, uint8_t byte) {
  for (auto &device : devices) {
    if (device->valid_for(addr)) {
      device->write8(addr, byte);
      return;
    }
  }
  spdlog::error("No device implemented for address: 0x{:02x}", addr);
}

void Mmu::write16(uint16_t addr, uint16_t word) {
  write8(addr, word & 0xff);
  write8(addr + 1, word >> 8);
}

uint8_t Mmu::read8(uint16_t addr) const {
  for (const auto &device : devices) {
    if (device->valid_for(addr)) {
      return device->read8(addr);
    }
  }
  std::unreachable();
}

uint16_t Mmu::read16(uint16_t addr) const {
  uint8_t lo = read8(addr);
  uint8_t hi = read8(addr + 1);
  return lo | (hi << 8);
}

void Mmu::reset_devices() {
  for (auto &device : devices) {
    device->reset();
  }
}
