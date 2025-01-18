#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "mmu_device.h"

class Mmu {
public:
  void clear_devices();
  void add_device(mmu_device_ptr device);

  void write8(uint16_t addr, uint8_t byte);
  void write16(uint16_t addr, uint16_t word);

  [[nodiscard]] uint8_t read8(uint16_t addr) const;
  [[nodiscard]] uint16_t read16(uint16_t addr) const;

  void reset_devices();

private:
  std::vector<mmu_device_ptr> devices;
};

