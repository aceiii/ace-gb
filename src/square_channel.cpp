#include <algorithm>
#include <utility>
#include <spdlog/spdlog.h>

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
  period.calculated = false;
  regs.fill(0);
}

void SquareChannel::write(AudioRegister reg, uint8_t value) {
  const auto idx = std::to_underlying(reg);
  const auto prev_direction = nrx0.period_sweep_direction;
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx0:
      if (!nrx0.period_sweep_direction && prev_direction && period.calculated) {
        enable_channel = false;
      }

      period.calculated = false;
      period.enabled = nrx0.period_sweep_pace || nrx0.period_sweep_step;
      if (!period.enabled) {
        period.timer = 0;
        period.current = 0;
      }
//      spdlog::info("NRx0: {:02x}, sweep_step:{}, sweep_direction:{}, sweep_pace:{}, length_counter:{}", value, static_cast<uint8_t>(nrx0.period_sweep_step), static_cast<uint8_t>(nrx0.period_sweep_direction), static_cast<uint8_t>(nrx0.period_sweep_pace), length_counter);
      break;
    case AudioRegister::NRx1:
      length_counter = kInitialLengthCounter - nrx1.initial_length_timer;
//      spdlog::info("NRx1: {:02x}, initial_length_timer: {}, length_counter:{}", value, static_cast<uint8_t>(nrx1.initial_length_timer), length_counter);
      break;
    case AudioRegister::NRx2:
//      spdlog::info("NRx2: {:02x}, initial_volume: {}, envelope_direction:{}, sweep_pace:{}", value, static_cast<uint8_t>(nrx2.initial_volume), static_cast<uint8_t>(nrx2.envelope_direction), static_cast<uint8_t>(nrx2.envelope_sweep_pace));
      volume = nrx2.initial_volume;
      envelope_timer = nrx2.envelope_sweep_pace;
      if (!nrx2.dac) {
        enable_channel = false;
      }
      break;
    case AudioRegister::NRx3:
//      spdlog::info("NRx3: {:02x}", value);
      break;
    case AudioRegister::NRx4:
//      spdlog::info("NRx4: {:02x}, trigger:{}, length_enable:{}, period:{}", value, static_cast<uint8_t>(nrx4.trigger), static_cast<uint8_t>(nrx4.length_enable), static_cast<uint8_t>(nrx4.period));
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
  enable_channel = nrx2.dac;
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
  period.calculated = false;
  if (!period.enabled) {
    period.timer = 0;
    period.current = 0;
    return;
  }

  period.current = frequency();
  period.timer = nrx0.period_sweep_pace;
  if (!period.timer) {
    period.timer = 8;
  }

  if (nrx0.period_sweep_step) {
    calc_sweep();
  }
}

bool SquareChannel::enabled() const {
//  spdlog::info("square:length_counter:{}, period_sweep_step:{}, period_sweep_pace:{}, enable_sweep:{}", length_counter, nrx0.period_sweep_step, nrx0.period_sweep_pace, enable_sweep);
  return enable_channel;
}

void SquareChannel::length_tick() {
  if (!nrx4.length_enable) {
    return;
  }

//  spdlog::info("length_tick:{}", length_counter);

  if (length_counter) {
//    spdlog::info("square channel length_tick: {} -> {}, enable_sweep:{}", length_counter, length_counter-1, enable_sweep);
    length_counter -= 1;
  }

  if (length_counter == 0 && enable_channel) {
//    spdlog::info("disable square channel: enable_sweep:{}, length_counter:{}", enable_sweep, length_counter);
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
  if (envelope_timer == 0) {
    envelope_timer = 8;
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
    period.timer = nrx0.period_sweep_pace;
    if (!period.timer) {
      period.timer = 8;
    }

    if (nrx0.period_sweep_pace) {
      auto new_frequency = calc_sweep();
      period.calculated = true;
      if (new_frequency <= 2047 && nrx0.period_sweep_step) {
        period.current = new_frequency;
        set_frequency(new_frequency);
        calc_sweep();
      }
    }
  }
}

uint16_t SquareChannel::calc_sweep() {
//  spdlog::info("square_channel calc_sweep: enable_sweep:{}, period_sweep_step:{}, period_sweep_pace:{}", enable_sweep, static_cast<uint8_t>(nrx0.period_sweep_step), static_cast<uint8_t>(nrx0.period_sweep_pace));
  auto val = period.current >> nrx0.period_sweep_step;
  if (nrx0.period_sweep_direction) {
    val = period.current - val;
  } else {
    val = period.current + val;
  }

  period.calculated = true;

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
