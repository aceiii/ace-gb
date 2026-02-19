#pragma once

#include <string>
#include <vector>

#include "mmu_device.h"
#include "interrupt_device.h"
#include "synced_device.hpp"

using LineCallback = std::function<void(const std::string &str)>;

class SerialDevice : public MmuDevice, public SyncedDevice {
public:
  explicit SerialDevice(InterruptDevice &interrupts);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void step();
  void OnTick() override;
  void trigger_callbacks();

  std::string_view line_buffer() const;
  void on_line(const LineCallback& callback);

private:
  InterruptDevice &interrupts;
  uint16_t clock = 0;
  uint16_t transfer_bytes = 0;
  uint8_t byte_buffer;
  std::string str_buffer;
  std::vector<LineCallback> callbacks;

  uint8_t sb;
  union {
    struct {
      uint8_t clock_select    : 1;
      uint8_t clock_speed     : 1;
      uint8_t unused          : 5;
      uint8_t transfer_enable : 1;
    };
    uint8_t val;
  } sc;

};
