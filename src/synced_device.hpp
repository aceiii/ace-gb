#pragma once


class SyncedDevice {
public:
  virtual ~SyncedDevice() = default;

  virtual void OnTick(bool double_speed) = 0;
};
