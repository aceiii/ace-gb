#pragma once

#include <string>
#include <vector>

#include "mmu_device.h"
#include "interrupt_device.h"

using LineCallback = std::function<void(const std::string &str)>;

class SerialDevice : public MmuDevice {
public:
  explicit SerialDevice(InterruptDevice &interrupts);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void execute(uint8_t cycles);
  std::string_view line_buffer() const;
  void on_line(const LineCallback& callback);

private:
  void trigger_callbacks();

  InterruptDevice &interrupts;
  uint8_t transfer_bytes = 0;
  uint8_t byte_buffer;
  std::string str_buffer;
  std::vector<LineCallback> callbacks;

  uint8_t sb;
  union {
    struct {
      uint8_t clock_select    : 1;
      uint8_t clock_speed     : 1;
      uint8_t                 : 5;
      uint8_t transfer_enable : 1;
    };
    uint8_t val;
  } sc;

};
