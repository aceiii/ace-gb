#include <algorithm>
#include <utility>

#include "square_channel.h"

using waveform = std::array<uint8_t, 8>;

std::array<waveform, 4> waveforms {{
  { 0, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 0, 0, 0, 0, 0, 1 },
  { 1, 0, 0, 0, 0, 1, 1, 1 },
  { 0, 1, 1, 1, 1, 1, 1, 0 },
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
  regs.fill(0);
}

void SquareChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  regs[idx] = value;

  if (reg == AudioRegister::NRx0) {
    if (!period.enabled && nrx0.period_sweep_pace) {
      period.enabled = true;
    } else if (nrx0.period_sweep_pace == 0) {
      period.enabled = false;
    }
  } else if (reg == AudioRegister::NRx1) {
    length_counter = nrx1.initial_length_timer;
  } else if (reg == AudioRegister::NRx2) {
    envelope_timer = nrx2.envelope_sweep_pace;
  } else if (reg == AudioRegister::NRx4) {
    if (nrx4.trigger) {
      trigger();
    }
  }
}

uint8_t SquareChannel::read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks[idx];
}

uint8_t SquareChannel::sample() const {
  if (!enable_channel) {
    return 0;
  }

  const auto &waveform = waveforms[nrx1.wave_duty];
  auto bit = waveform[duty_step];

  if (bit) {
    return volume;
  }

  return 0;
}

void SquareChannel::tick() {
//  if (period.timer) {
//    period.timer -= 1;
//  }
//
//  if (period.timer == 0) {
//    duty_step = (duty_step + 1) % 8;
//    period.timer = (2048 - period.current) * 4;
//  }
}

void SquareChannel::trigger() {
  duty_step = 0;
  enable_channel = nrx2.dac;
  period.current = (nrx4.period << 8) | nrx3;
  envelope_timer = nrx2.envelope_sweep_pace;
  volume = nrx2.initial_volume;

  if (length_counter == 0) {
    length_counter = kInitialLengthCounter;
  }

  if (!enable_sweep) {
    return;
  }

  period.timer = (2048 - period.current) * 4;
  if (nrx0.period_sweep_step) {
    // NOTE: frequency calculation and overflow check

    auto frequency = period.current >> nrx0.period_sweep_step;
    if (frequency > 0x7ff) {
      enable_channel = false;
    }
  }
}

void SquareChannel::length_tick() {
  if (!length_counter || !nrx4.length_enable) {
    return;
  }

  if (--length_counter == 0) {
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

  if (envelope_timer == 0) {
    int8_t change = nrx2.envelope_direction ? 1 : -1;
    volume = std::clamp(volume + change, 0, 0xf);

    if (nrx2.envelope_direction && volume < 0xf) {
      volume += 1;
    } else if (!nrx2.envelope_direction && volume > 0){
      volume -= 1;
    }

    envelope_timer = nrx2.envelope_sweep_pace;
  }
}

void SquareChannel::sweep_tick() {
  if (!enable_sweep) {
    return;
  }

  if (period.timer) {
    period.timer -= 1;
  }

  if (period.timer == 0) {
   auto new_value = period.current;
    if (nrx0.period_sweep_direction) {
      new_value -= period.current >> nrx0.period_sweep_step;
    } else {
      new_value += period.current >> nrx0.period_sweep_step;
    }

    period.current = new_value;
    period.timer = (2048 - period.current) * 4;
    nrx3 = period.current & 0xff;
    nrx4.period = (period.current >> 8) & 0xff;

    if (period.current > 0x7ff) {
      enable_channel = false;
    }
  }
}
