#include <algorithm>
#include <utility>

#include "square_channel.h"

std::array<uint8_t, 4> waveforms {{
  0b00000001,
  0b10000001,
  0b10000111,
  0b01111110,
}};

constexpr std::array<uint8_t, 5> defaultMask(bool sweep) {
  if (sweep) {
    return { 0x80, 0x3f, 0x00, 0xff, 0xbf };
  }
  return { 0xff, 0x3f, 0x00, 0xff, 0xbf };
}

constexpr auto kInitialLengthCounter = 64;

SquareChannel::SquareChannel(bool sweep): enable_sweep { sweep }, masks { defaultMask(sweep) } {
}

void SquareChannel::reset() {
  enable_channel = false;
  length_counter = 0;
  envelope_timer = 0;
  timer = 0;
  volume = 0;
  duty_step = 0;
  period.enabled = false;
  period.timer = 0;
  period.current = 0;
  regs.fill(0);
}

void SquareChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx0:
      if (!period.enabled && nrx0.period_sweep_pace) {
        period.enabled = true;
      } else if (nrx0.period_sweep_pace == 0) {
        period.enabled = false;
      }
      break;
    case AudioRegister::NRx1:
      length_counter = nrx1.initial_length_timer;
      break;
    case AudioRegister::NRx2:
      volume = nrx2.initial_volume;
      envelope_timer = nrx2.envelope_sweep_pace;
      break;
    case AudioRegister::NRx4:
      if (nrx4.trigger) {
        trigger();
      }
      break;
    default: break;
  }
}

uint8_t SquareChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

float SquareChannel::sample() const {
  if (!enable_channel || !nrx2.dac) {
    return 0.0f;
  }

  const auto &waveform = waveforms[nrx1.wave_duty];
  auto bit = (waveform >> (7 - duty_step)) & 0b1;

  if (bit) {
    return (static_cast<float>(volume) / 7.5f) - 1.0f;
  }

  return 0.0f;
}

void SquareChannel::tick() {
  if (timer) {
    timer -= 1;
  }

  if (timer == 0) {
    duty_step = (duty_step + 1) % 8;
     timer = (2048 - frequency()) * 4;
  }
}

void SquareChannel::trigger() {
  enable_channel = true;
  envelope_timer = nrx2.envelope_sweep_pace;
  volume = nrx2.initial_volume;
  timer = (2048 - frequency()) * 4;
  duty_step = 0;

  if (length_counter == 0) {
    length_counter = kInitialLengthCounter;
  }

  if (!enable_sweep) {
    return;
  }

  period.enabled = nrx0.period_sweep_step || nrx0.period_sweep_pace;
  period.current = frequency();
  period.timer = nrx0.period_sweep_pace;
  if (nrx0.period_sweep_step) {
    calc_sweep();
  }
}

void SquareChannel::length_tick() {
  if (length_counter) {
    length_counter -= 1;
  }

  if (!nrx4.length_enable) {
    return;
  }

  if (length_counter == 0) {
    enable_channel = false;
  }
}

void SquareChannel::envelope_tick() {
  if (!nrx2.envelope_sweep_pace) {
    return;
  }

  if (envelope_timer) {
    envelope_timer -= 1;
  }

  if (envelope_timer != 0) {
    return;
  }

  if (nrx2.envelope_direction && volume < 0xf) {
    volume += 1;
  } else if (!nrx2.envelope_direction && volume > 0) {
    volume -= 1;
  }

  envelope_timer = nrx2.envelope_sweep_pace;
}

void SquareChannel::sweep_tick() {
  if (!enable_sweep) {
    return;
  }

  if (period.timer) {
    period.timer -= 1;
  }

  if (period.timer == 0 && nrx0.period_sweep_step) {
    auto new_frequency = calc_sweep();
    if (new_frequency <= 2047 && nrx0.period_sweep_step) {
      period.current = new_frequency;
      set_frequency(new_frequency);
      calc_sweep();
    }
    period.timer = nrx0.period_sweep_pace;
  }
}

uint16_t SquareChannel::calc_sweep() {
  uint16_t val = period.current >> nrx0.period_sweep_step;
  if (nrx0.period_sweep_direction) {
    val = period.current - val;
  } else {
    val = period.current + val;
  }

  if (val > 2047) {
    enable_channel = false;
  }

  return val;
}

uint16_t SquareChannel::frequency() const {
  return nrx3 | (nrx4.period << 8);
}

void SquareChannel::set_frequency(uint16_t freq) {
  nrx3 = freq & 0xff;
  nrx4.period = (freq >> 8) & 0b111;
}
