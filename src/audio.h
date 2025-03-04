#pragma once

#include <array>
#include <utility>

#include "io.h"
#include "mmu_device.h"
#include "synced_device.h"

constexpr int kAudioStart = std::to_underlying(IO::NR10);
constexpr int kAudioEnd = std::to_underlying(IO::LCDC) - 1;
constexpr int kAudioSize = kAudioEnd - kAudioStart + 1;

class Audio : public MmuDevice, public SyncedDevice {
public:
  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void on_tick() override;

  void get_samples(float *samples, size_t num_samples, size_t num_channels);

private:
  union {
    std::array<uint8_t, kAudioSize> ram {};

    struct {
      struct {
        uint8_t step: 3;
        uint8_t direction: 1;
        uint8_t pace: 3;
        uint8_t unused: 1;
      } nr10;

      struct {
        uint8_t initial_length_timer: 6;
        uint8_t wave_duty: 2;
      } nr11;

      struct {
        uint8_t sweep_pace: 3;
        uint8_t env_dir: 1;
        uint8_t initial_volume: 4;
      } nr12;

      uint8_t nr13;

      struct {
        uint8_t period: 3;
        uint8_t unused: 3;
        uint8_t length_enable: 1;
        uint8_t trigger: 1;
      } nr14;

      uint8_t nr20; // unused

      struct {
        uint8_t initial_length_timer: 6;
        uint8_t wave_duty: 2;
      } nr21;

      struct {
        uint8_t sweep_pace: 3;
        uint8_t env_dir: 1;
        uint8_t initial_volume: 4;
      } nr22;

      uint8_t nr23;

      struct {
        uint8_t period: 3;
        uint8_t unused: 3;
        uint8_t length_enable: 1;
        uint8_t trigger: 1;
      } nr24;

      struct {
        uint8_t unused : 7;
        uint8_t dac_enable : 1;
      } nr30;

      uint8_t nr31;

      struct {
        uint8_t unused : 5;
        uint8_t output_level : 1;
        uint8_t unused2 : 1;
      } nr32;

      uint8_t nr33;

      struct {
        uint8_t period : 3;
        uint8_t unused : 3;
        uint8_t length_enable : 1;
        uint8_t trigger : 1;
      } nr34;

      uint8_t nr40; // unused

      union {
        uint8_t val;
        struct {
          uint8_t initial_length_timer : 6;
          uint8_t unused : 2;
        };
      } nr41;

      uint8_t nr42;

      struct {
        uint8_t clock_divider : 3;
        uint8_t lfsr_width : 1;
        uint8_t clock_shift : 4;
      } nr43;

      struct {
        uint8_t unused : 6;
        uint8_t length_enable : 1;
        uint8_t trigger : 1;
      } nr44;

      struct {
        uint8_t right_volume : 3;
        uint8_t vin_right : 1;
        uint8_t left_volume : 3;
        uint8_t vin_left : 1;
      } nr50;

      struct {
        uint8_t ch1_right: 1;
        uint8_t ch2_right: 1;
        uint8_t ch3_right: 1;
        uint8_t ch4_right: 1;
        uint8_t ch1_left: 1;
        uint8_t ch2_left: 1;
        uint8_t ch3_left: 1;
        uint8_t ch4_left: 1;
      } nr51;

      struct {
        uint8_t ch1: 1;
        uint8_t ch2: 1;
        uint8_t ch3: 1;
        uint8_t ch4: 1;
        uint8_t unused: 3;
        uint8_t audio: 1;
      } nr52;

      std::array<uint8_t, 9> unused;

      std::array<uint16_t, 16> wave_pattern_ram;
    };
  };
};
