#pragma once

#include <array>
#include <tuple>
#include <utility>
#include <vector>

#include "io.hpp"
#include "mmu_device.hpp"
#include "synced_device.hpp"
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
constexpr int kSampleBufferMaxSize = 4096 * 4;

struct audio_config {
  int sample_rate;
  int buffer_size;
  int num_channels;
};

class Audio : public MmuDevice, public SyncedDevice {
public:
  explicit Audio(Timer &timer, audio_config cfg);

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;
  void PowerOff();

  void OnTick() override;

  void GetSamples(std::vector<float> &out_buffer);

  bool IsChannelEnabled(AudioChannelID channel) const;
  void ToggleChannel(AudioChannelID channel, bool enable);

private:
  std::tuple<float, float> Sample() const;

private:
  Timer &timer_;
  audio_config config_;

  uint8_t frame_sequencer_ {};
  uint16_t frame_sequencer_counter_ {};
  uint16_t sample_timer_ {};
  std::array<bool, 5> enable_channel_ {{ true, true, true, true, true }};
  std::array<float, kSampleBufferMaxSize> sample_buffer_ {};
  uint16_t buffer_write_idx_ {};
  uint16_t buffer_read_idx_ {};

  union {
    uint8_t val;
    struct {
      uint8_t right_volume: 3;
      uint8_t vin_right: 1;
      uint8_t left_volume: 3;
      uint8_t vin_left: 1;
    };
  } nr50_ {};

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
  } nr51_ {};

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
  } nr52_ {};

  SquareChannel ch1_ {true};
  SquareChannel ch2_ {false};
  WaveChannel ch3_ {};
  NoiseChannel ch4_ {};

};
