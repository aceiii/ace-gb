#pragma once

#include <array>
#include <tuple>
#include <utility>
#include <vector>

#include "io.h"
#include "mmu_device.h"
#include "synced_device.h"
#include "timer.h"
#include "square_channel.h"
#include "noise_channel.h"
#include "wave_channel.h"

enum class AudioChannelID {
  CH1,
  CH2,
  CH3,
  CH4,
  MASTER,
};

constexpr int kAudioStart = std::to_underlying(IO::NR10);
constexpr int kAudioEnd = std::to_underlying(IO::LCDC) - 1;
constexpr int kAudioSize = kAudioEnd - kAudioStart + 1;

struct audio_config {
  int sample_rate;
  int buffer_size;
  int num_channels;
};

class Audio : public MmuDevice, public SyncedDevice {
public:
  explicit Audio(Timer &timer, audio_config cfg);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;
  void poweroff();

  void on_tick() override;

  void get_samples(float *samples, size_t num_samples, size_t num_channels);

  bool channel_enabled(AudioChannelID channel) const;
  void toggle_channel(AudioChannelID channel, bool enable);

private:
  std::tuple<float, float> sample();

private:
  Timer &timer;
  audio_config config;

  uint8_t prev_bit {};
  uint8_t frame_sequencer {};
  uint16_t sample_timer {};
  std::vector<float> sample_buffer {};
  std::array<bool, 5> enable_channel {{ true, true, true, true, true }};

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

  SquareChannel ch1 {true};
  SquareChannel ch2 {false};
  WaveChannel ch3 {};
  NoiseChannel ch4 {};

};
