#pragma once

#include <array>
#include <utility>

#include "io.h"
#include "mmu_device.h"
#include "synced_device.h"
#include "timer.h"
#include "square_channel.h"
#include "noise_channel.h"
#include "wave_channel.h"

constexpr int kAudioStart = std::to_underlying(IO::NR10);
constexpr int kAudioEnd = std::to_underlying(IO::LCDC) - 1;
constexpr int kAudioSize = kAudioEnd - kAudioStart + 1;

class Audio : public MmuDevice, public SyncedDevice {
public:
  explicit Audio(Timer &timer);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void on_tick() override;

  void get_samples(float *samples, size_t num_samples, size_t num_channels);

private:
  Timer &timer;

  uint8_t div_apu;

  union {
    uint8_t val;
    struct {
      uint8_t right_volume: 3;
      uint8_t vin_right: 1;
      uint8_t left_volume: 3;
      uint8_t vin_left: 1;
    };
  } nr50;

  union {
    uint8_t val;
    struct {
      uint8_t ch1_right: 1;
      uint8_t ch2_right: 1;
      uint8_t ch3_right: 1;
      uint8_t ch4_right: 1;
      uint8_t ch1_left: 1;
      uint8_t ch2_left: 1;
      uint8_t ch3_left: 1;
      uint8_t ch4_left: 1;
    };
  } nr51;

  union {
    uint8_t val;
    struct {
      uint8_t ch1: 1;
      uint8_t ch2: 1;
      uint8_t ch3: 1;
      uint8_t ch4: 1;
      uint8_t unused: 3;
      uint8_t audio: 1;
    };
  } nr52;

  SquareChannel ch1 {};
  SquareChannel ch2 {};
  WaveChannel ch3 {};
  NoiseChannel ch4 {};

};
