#include <spdlog/spdlog.h>

#include "mmu.hpp"

void Mmu::ClearDevices() {
  devices_.clear();
}

void Mmu::AddDevice(MmuDevicePtr device) {
  devices_.emplace_back(device);
}

void Mmu::Write8(uint16_t addr, uint8_t byte) {
  for (auto& device : devices_) {
    if (device->IsValidFor(addr)) {
      device->Write8(addr, byte);
      return;
    }
  }
  spdlog::error("No device implemented for address: 0x{:02x}", addr);
}

uint8_t Mmu::Read8(uint16_t addr) const {
  for (const auto& device : devices_) {
    if (device->IsValidFor(addr)) {
      return device->Read8(addr);
    }
  }
  std::unreachable();
}

void Mmu::ResetDevices() {
  for (auto& device : devices_) {
    device->Reset();
  }
}
