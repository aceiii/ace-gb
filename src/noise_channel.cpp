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

void NoiseChannel::reset() {
  enable_channel = false;
  length_counter = 0;
  envelope_timer = 0;
  timer = 0;
  lfsr.bytes = 0;
  volume = 0;
  regs.fill(0);
}

void NoiseChannel::poweroff() {
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

void NoiseChannel::write(AudioRegister reg, uint8_t value) {
  auto prev_clock_divider = nrx3.clock_divider;

  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx1:
      length_counter = kInitialLengthCounter - nrx1.initial_length_timer;
//      spdlog::info("NRx1: {:02x}, initial_length_timer: {}, length_counter:{}", value, static_cast<uint8_t>(nrx1.initial_length_timer), length_counter);
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
//      spdlog::info("NRx4: {:02x}, trigger:{}, length_enable:{}", value, static_cast<uint8_t>(nrx4.trigger), static_cast<uint8_t>(nrx4.length_enable));
      if (nrx4.trigger) {
        trigger();
      }
      break;
    default:
      break;
  }
}

uint8_t NoiseChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

float NoiseChannel::sample() const {
  if (!enable_channel || !nrx2.dac) {
    return 0.0f;
  }

  auto bit = !(lfsr.val & 0b1);
  auto vol = static_cast<float>(volume) / 15.0f;
  return bit ? vol : -vol;
}

void NoiseChannel::tick() {
  if (timer) {
    timer -= 1;
  }

  if (timer == 0) {
    lfsr.temp = ~((lfsr.val & 0b1) ^ ((lfsr.val >> 1) & 0b1)) & 0b1;
    if (nrx3.lfsr_width) {
      lfsr.bytes = (lfsr.bytes & ~0x80) | (lfsr.temp << 7);
    }
    lfsr.bytes >>= 1;
    timer = clock_divisor(nrx3.clock_divider) << nrx3.clock_shift;
  }
}

void NoiseChannel::trigger() {
  enable_channel = nrx2.dac;

  if (length_counter == 0) {
    length_counter = kInitialLengthCounter;
  }

  timer = clock_divisor(nrx3.clock_divider) << nrx3.clock_shift;
  envelope_timer = nrx2.sweep_pace;
  volume = nrx2.initial_volume;
  lfsr.bytes = 0; //0xff;
}

bool NoiseChannel::enabled() const {
  return enable_channel;
}

void NoiseChannel::length_tick() {
  if (!nrx4.length_enable) {
    return;
  }

  if (length_counter) {
//    spdlog::info("noise channel length_tick: {} -> {}", length_counter, length_counter-1);
    length_counter -= 1;
  }

  if (length_counter == 0) {
//    spdlog::info("disable noise channel: length_counter:{}", length_counter);
    enable_channel = false;
  }
}

void NoiseChannel::envelope_tick() {
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

void NoiseChannel::sweep_tick() {
}
