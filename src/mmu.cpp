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
}

void Mmu::write16(uint16_t addr, uint16_t word) {
  for (auto &device : devices) {
    if (device->valid_for(addr)) {
      device->write16(addr, word);
      return;
    }
  }
}

uint8_t Mmu::read8(uint16_t addr) const {
  for (const auto &device : devices) {
    if (device->valid_for(addr)) {
      return device->read8(addr);
    }
  }
  return 0;
}

uint16_t Mmu::read16(uint16_t addr) const {
  for (const auto &device : devices) {
    if (device->valid_for(addr)) {
      return device->read16(addr);
    }
  }
  return 0;
}

void Mmu::reset_devices() {
  for (auto &device : devices) {
    device->reset();
  }
}
