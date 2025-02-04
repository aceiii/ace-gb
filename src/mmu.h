#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "mmu_device.h"

class Mmu {
public:
  void clear_devices();
  void add_device(mmu_device_ptr device);

  [[nodiscard]] uint8_t read8(uint16_t addr) const;
  void write8(uint16_t addr, uint8_t byte);

  void reset_devices();

private:
  std::vector<mmu_device_ptr> devices;
};

