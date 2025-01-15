#include <algorithm>
#include <spdlog/spdlog.h>

#include "mmu.h"
#include "memory.h"

void MMU::add_device(addr_range range, std::shared_ptr<IMMUDevice> &&device) {
  devices_.emplace_back(range, device);
}

void MMU::write8(uint16_t addr, uint8_t byte) {
  for (auto &[range, device] : devices_) {
    if (addr >= range.start && addr < range.end) {
      device->write8(addr, byte);
      return;
    }
  }
}

void MMU::write16(uint16_t addr, uint16_t word) {
  for (auto &[range, device] : devices_) {
    if (addr >= range.start && addr < range.end) {
      device->write16(addr, word);
      return;
    }
  }
}

uint8_t MMU::read8(uint16_t addr) const {
  for (auto &[range, device] : devices_) {
    if (addr >= range.start && addr < range.end) {
      return device->read8(addr);
    }
  }
  return 0;
}

uint16_t MMU::read16(uint16_t addr) const {
  for (auto &[range, device] : devices_) {
    if (addr >= range.start && addr < range.end) {
      return device->read16(addr);
    }
  }
  return 0;
}

void MMU::reset_devices() {
  for (auto &[addr, device] : devices_) {
    device->reset();
  }
}
