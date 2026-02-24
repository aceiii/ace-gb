#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

#include "mmu.hpp"

void Mmu::ClearDevices() {
  devices_.clear();
}

void Mmu::AddDevice(MmuDevicePtr device) {
  devices_.emplace_back(device);
}

void Mmu::Write8(u16 addr, u8 byte) {
  ZoneScoped;
  for (auto& device : devices_) {
    if (device->IsValidFor(addr)) {
      device->Write8(addr, byte);
      return;
    }
  }
  spdlog::error("No device implemented for address: 0x{:02x}", addr);
}

u8 Mmu::Read8(u16 addr) const {
  ZoneScoped;
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
