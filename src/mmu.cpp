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

uint8_t Mmu::read8(uint16_t addr) const {
  for (const auto &device : devices) {
    if (device->valid_for(addr)) {
      if (addr == 0xff03) {
        spdlog::info("device: {}", std::find(devices.begin(), devices.end(), device) - devices.begin());
      }
      return device->read8(addr);
    }
  }
  std::unreachable();
}

void Mmu::reset_devices() {
  for (auto &device : devices) {
    device->reset();
  }
}
