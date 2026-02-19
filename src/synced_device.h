#pragma once


class SyncedDevice {
public:
  virtual ~SyncedDevice() = default;

  virtual void OnTick() = 0;
};
