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

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void Step();
  void OnTick() override;
  void TriggerCallbacks();

  std::string_view LineBuffer() const;
  void OnLine(const LineCallback& callback);

private:
  InterruptDevice& interrupts_;
  u16 clock_ = 0;
  u16 transfer_bytes_ = 0;
  u8 byte_buffer_;
  std::string str_buffer_;
  std::vector<LineCallback> callbacks_;

  u8 sb_;
  union {
    struct {
      u8 clock_select    : 1;
      u8 clock_speed     : 1;
      u8 unused          : 5;
      u8 transfer_enable : 1;
    };
    u8 val;
  } sc_;

};
