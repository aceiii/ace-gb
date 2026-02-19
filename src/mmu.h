#pragma once

#include <vector>

#include "mmu_device.hpp"

class Mmu {
public:
  void ClearDevices();
  void AddDevice(MmuDevicePtr device);

  [[nodiscard]] uint8_t Read8(uint16_t addr) const;
  void Write8(uint16_t addr, uint8_t byte);

  void ResetDevices();

private:
  std::vector<MmuDevicePtr> devices_;
};

