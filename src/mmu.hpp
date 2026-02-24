#pragma once

#include <vector>

#include "mmu_device.hpp"

class Mmu {
public:
  void ClearDevices();
  void AddDevice(MmuDevicePtr device);

  [[nodiscard]] u8 Read8(u16 addr) const;
  void Write8(u16 addr, u8 byte);

  void ResetDevices();

private:
  std::vector<MmuDevicePtr> devices_;
};

