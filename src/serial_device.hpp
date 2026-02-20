#pragma once

#include <string>
#include <vector>

#include "mmu_device.hpp"
#include "interrupt_device.hpp"
#include "synced_device.hpp"

using LineCallback = std::function<void(std::string_view str)>;

class SerialDevice : public MmuDevice, public SyncedDevice {
public:
  explicit SerialDevice(InterruptDevice& interrupts);

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void Step();
  void OnTick() override;
  void TriggerCallbacks();

  std::string_view LineBuffer() const;
  void OnLine(const LineCallback& callback);

private:
  InterruptDevice& interrupts_;
  uint16_t clock_ = 0;
  uint16_t transfer_bytes_ = 0;
  uint8_t byte_buffer_;
  std::string str_buffer_;
  std::vector<LineCallback> callbacks_;

  uint8_t sb_;
  union {
    struct {
      uint8_t clock_select    : 1;
      uint8_t clock_speed     : 1;
      uint8_t unused          : 5;
      uint8_t transfer_enable : 1;
    };
    uint8_t val;
  } sc_;

};
