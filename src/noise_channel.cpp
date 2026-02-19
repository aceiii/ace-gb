#include <spdlog/spdlog.h>
#include <utility>

#include "noise_channel.h"

constexpr auto kInitialLengthCounter = 64;

constexpr std::array<uint8_t, 5> kDefaultMasks {{ 0xff, 0xff, 0x00, 0x00, 0xbf }};

uint8_t clock_divisor(uint8_t div) {
  if (!div) {
    return 8;
  }
  return div * 16;
}

NoiseChannel::NoiseChannel(): masks { kDefaultMasks } {
}

void NoiseChannel::Reset() {
  enable_channel = false;
  length_counter = 0;
  envelope_timer = 0;
  timer = 0;
  lfsr.bytes = 0;
  volume = 0;
  regs.fill(0);
}

void NoiseChannel::PowerOff() {
  enable_channel = false;
  length_counter = 0;
  envelope_timer = 0;
  timer = 0;
  lfsr.bytes = 0;
  volume = 0;

  auto initial_length_timer = nrx1.initial_length_timer;
  regs.fill(0);
  nrx1.initial_length_timer = initial_length_timer;
}

void NoiseChannel::Write(AudioRegister reg, uint8_t value) {
  auto prev_clock_divider = nrx3.clock_divider;

  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx1:
      length_counter = kInitialLengthCounter - nrx1.initial_length_timer;
      break;
    case AudioRegister::NRx2:
      volume = nrx2.initial_volume;
      envelope_timer = nrx2.sweep_pace;
      if (!nrx2.dac) {
        enable_channel = false;
      }
      break;
    case AudioRegister::NRx3:
      timer = clock_divisor(prev_clock_divider) << nrx3.clock_shift;
      break;
    case AudioRegister::NRx4:
      if (nrx4.trigger) {
        Trigger();
      }
      break;
    default:
      break;
  }
}

uint8_t NoiseChannel::Read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

float NoiseChannel::Sample() const {
  if (!enable_channel || !nrx2.dac) {
    return 0.0f;
  }

  auto bit = (lfsr.val & 0b1);
  auto vol = static_cast<float>(volume) / 15.0f;
  return bit ? -vol : vol;
}

void NoiseChannel::Tick() {
  if (timer) {
    timer -= 1;
  }

  if (timer == 0) {
    lfsr.temp = (lfsr.bit0 ^ lfsr.bit1) & 1;
    if (nrx3.lfsr_width) {
      lfsr.mid = lfsr.temp;
    }
    lfsr.bytes >>= 1;
    lfsr.temp = 0;
    timer = clock_divisor(nrx3.clock_divider) << nrx3.clock_shift;
  }
}

void NoiseChannel::Trigger() {
  enable_channel = nrx2.dac;

  if (length_counter == 0) {
    length_counter = kInitialLengthCounter;
  }

  timer = clock_divisor(nrx3.clock_divider) << nrx3.clock_shift;
  envelope_timer = nrx2.sweep_pace;
  volume = nrx2.initial_volume;
  lfsr.bytes = 0x7fff;
}

bool NoiseChannel::IsEnabled() const {
  return enable_channel;
}

void NoiseChannel::TickLength() {
  if (!nrx4.length_enable) {
    return;
  }

  if (length_counter) {
    length_counter -= 1;
  }

  if (length_counter == 0) {
    enable_channel = false;
  }
}

void NoiseChannel::TickEvenlope() {
  if (!nrx2.sweep_pace) {
    return;
  }

  if (envelope_timer) {
    envelope_timer -= 1;
  }

  if (envelope_timer == 0) {
    if (nrx2.env_dir && volume < 0xf) {
      volume += 1;
    } else if (!nrx2.env_dir && volume > 0) {
      volume -= 1;
    }
    envelope_timer = nrx2.sweep_pace;
  }
}

void NoiseChannel::TickSweep() {
}
