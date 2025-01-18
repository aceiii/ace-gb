#include "mmu.h"

void MMU::add_device(std::shared_ptr<IMMUDevice> &&device) {
  devices_.emplace_back(device);
}

void MMU::write8(uint16_t addr, uint8_t byte) {
  for (auto &device : devices_) {
    if (device->valid_for(addr)) {
      device->write8(addr, byte);
      return;
    }
  }
}

void MMU::write16(uint16_t addr, uint16_t word) {
  for (auto &device : devices_) {
    if (device->valid_for(addr)) {
      device->write16(addr, word);
      return;
    }
  }
}

uint8_t MMU::read8(uint16_t addr) const {
  for (const auto &device : devices_) {
    if (device->valid_for(addr)) {
      return device->read8(addr);
    }
  }
  return 0;
}

uint16_t MMU::read16(uint16_t addr) const {
  for (const auto &device : devices_) {
    if (device->valid_for(addr)) {
      return device->read16(addr);
    }
  }
  return 0;
}

void MMU::reset_devices() {
  for (auto &device : devices_) {
    device->reset();
  }
}
