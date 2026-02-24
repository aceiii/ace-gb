#include <algorithm>
#include <utility>
#include <spdlog/spdlog.h>

#include "square_channel.hpp"

std::array<u8, 4> waveforms {{
  0b00000001,
  0b10000001,
  0b10000111,
  0b01111110,
}};

constexpr std::array<u8, 5> defaultMask(bool sweep) {
  if (sweep) {
    return { 0x80, 0x3f, 0x00, 0xff, 0xbf };
  }
  return { 0xff, 0x3f, 0x00, 0xff, 0xbf };
}

constexpr auto kInitialLengthCounter = 64;

SquareChannel::SquareChannel(bool sweep): enable_sweep_ { sweep }, masks_ { defaultMask(sweep) } {
}

void SquareChannel::Reset() {
  enable_channel_ = false;
  length_counter_ = 0;
  envelope_timer_ = 0;
  timer_ = 0;
  volume_ = 0;
  duty_step_ = 0;
  period_.enabled = false;
  period_.timer = 0;
  period_.current = 0;
  period_.calculated = false;
  regs.fill(0);
}

void SquareChannel::PowerOff() {
  enable_channel_ = false;
  length_counter_ = 0;
  envelope_timer_ = 0;
  timer_ = 0;
  volume_ = 0;
  duty_step_ = 0;
  period_.enabled = false;
  period_.timer = 0;
  period_.current = 0;
  period_.calculated = false;
  regs.fill(0);
}

void SquareChannel::Write(AudioRegister reg, u8 value) {
  const auto idx = std::to_underlying(reg);
  const auto prev_direction = nrx0.period_sweep_direction;
  regs[idx] = value;

  switch (reg) {
    case AudioRegister::NRx0:
      if (!nrx0.period_sweep_direction && prev_direction && period_.calculated) {
        enable_channel_ = false;
      }

      period_.calculated = false;
      period_.enabled = nrx0.period_sweep_pace || nrx0.period_sweep_step;
      if (!period_.enabled) {
        period_.timer = 0;
        period_.current = 0;
      }
//      spdlog::info("NRx0: {:02x}, sweep_step:{}, sweep_direction:{}, sweep_pace:{}, length_counter:{}", value, static_cast<u8>(nrx0.period_sweep_step), static_cast<u8>(nrx0.period_sweep_direction), static_cast<u8>(nrx0.period_sweep_pace), length_counter);
      break;
    case AudioRegister::NRx1:
      length_counter_ = kInitialLengthCounter - nrx1.initial_length_timer;
//      spdlog::info("NRx1: {:02x}, initial_length_timer: {}, length_counter:{}", value, static_cast<u8>(nrx1.initial_length_timer), length_counter);
      break;
    case AudioRegister::NRx2:
//      spdlog::info("NRx2: {:02x}, initial_volume: {}, envelope_direction:{}, sweep_pace:{}", value, static_cast<u8>(nrx2.initial_volume), static_cast<u8>(nrx2.envelope_direction), static_cast<u8>(nrx2.envelope_sweep_pace));
      volume_ = nrx2.initial_volume;
      envelope_timer_ = nrx2.envelope_sweep_pace;
      if (!nrx2.dac) {
        enable_channel_ = false;
      }
      break;
    case AudioRegister::NRx3:
//      spdlog::info("NRx3: {:02x}", value);
      break;
    case AudioRegister::NRx4:
//      spdlog::info("NRx4: {:02x}, trigger:{}, length_enable:{}, period:{}", value, static_cast<u8>(nrx4.trigger), static_cast<u8>(nrx4.length_enable), static_cast<u8>(nrx4.period));
      if (nrx4.trigger) {
        Trigger();
      }
      break;
    default: break;
  }
}

u8 SquareChannel::Read(AudioRegister reg) const {
  const auto idx = std::to_underlying(reg);
  return regs[idx] | masks_[idx];
}

float SquareChannel::Sample() const {
  if (!enable_channel_ || !nrx2.dac) {
    return 0.0f;
  }

  const auto& waveform = waveforms[nrx1.wave_duty];
  auto bit = (waveform >> (7 - duty_step_)) & 0b1;
  auto vol = static_cast<float>(volume_) / 15.0f;
  return bit ? vol : -vol;
}

void SquareChannel::Tick() {
  if (!IsEnabled()) {
    return;
  }

  if (timer_) {
    timer_ -= 1;
  }

  if (timer_ == 0) {
    duty_step_ = (duty_step_ + 1) % 8;
    timer_ = (2048 - GetFrequency()) * 4;
  }
}

void SquareChannel::Trigger() {
  enable_channel_ = nrx2.dac;
  envelope_timer_ = nrx2.envelope_sweep_pace;
  volume_ = nrx2.initial_volume;
  timer_ = (2048 - GetFrequency()) * 4;
  duty_step_ = 0;

  if (length_counter_ == 0) {
    length_counter_ = kInitialLengthCounter;
  }

  if (!enable_sweep_) {
    return;
  }

  period_.enabled = nrx0.period_sweep_step || nrx0.period_sweep_pace;
  period_.calculated = false;
  if (!period_.enabled) {
    period_.timer = 0;
    period_.current = 0;
    return;
  }

  period_.current = GetFrequency();
  period_.timer = nrx0.period_sweep_pace;
  if (!period_.timer) {
    period_.timer = 8;
  }

  if (nrx0.period_sweep_step) {
    CalcSweep();
  }
}

bool SquareChannel::IsEnabled() const {
//  spdlog::info("square:length_counter:{}, period_sweep_step:{}, period_sweep_pace:{}, enable_sweep:{}", length_counter, nrx0.period_sweep_step, nrx0.period_sweep_pace, enable_sweep);
  return enable_channel_;
}

void SquareChannel::TickLength() {
  if (!nrx4.length_enable) {
    return;
  }

//  spdlog::info("length_tick:{}", length_counter);

  if (length_counter_) {
//    spdlog::info("square channel length_tick: {} -> {}, enable_sweep:{}", length_counter, length_counter-1, enable_sweep);
    length_counter_ -= 1;
  }

  if (length_counter_ == 0 && enable_channel_) {
//    spdlog::info("disable square channel: enable_sweep:{}, length_counter:{}", enable_sweep, length_counter);
    enable_channel_ = false;
  }
}

void SquareChannel::TickEnvenlope() {
  if (!nrx2.envelope_sweep_pace) {
    return;
  }

  if (envelope_timer_) {
    envelope_timer_ -= 1;
  }

  if (envelope_timer_ != 0) {
    return;
  }

  if (nrx2.envelope_direction && volume_ < 0xf) {
    volume_ += 1;
  } else if (!nrx2.envelope_direction && volume_ > 0) {
    volume_ -= 1;
  }

  envelope_timer_ = nrx2.envelope_sweep_pace;
  if (envelope_timer_ == 0) {
    envelope_timer_ = 8;
  }
}

void SquareChannel::TickSweep() {
  if (!enable_sweep_) {
    return;
  }

  if (period_.timer) {
    period_.timer -= 1;
  }

  if (period_.timer == 0) {
    period_.timer = nrx0.period_sweep_pace;
    if (!period_.timer) {
      period_.timer = 8;
    }

    if (nrx0.period_sweep_pace) {
      auto new_frequency = CalcSweep();
      period_.calculated = true;
      if (new_frequency <= 2047 && nrx0.period_sweep_step) {
        period_.current = new_frequency;
        SetFrequency(new_frequency);
        CalcSweep();
      }
    }
  }
}

u16 SquareChannel::CalcSweep() {
//  spdlog::info("square_channel calc_sweep: enable_sweep:{}, period_sweep_step:{}, period_sweep_pace:{}", enable_sweep, static_cast<u8>(nrx0.period_sweep_step), static_cast<u8>(nrx0.period_sweep_pace));
  auto val = period_.current >> nrx0.period_sweep_step;
  if (nrx0.period_sweep_direction) {
    val = period_.current - val;
  } else {
    val = period_.current + val;
  }

  period_.calculated = true;

  if (val > 2047) {
    enable_channel_ = false;
  }

  return val;
}

u16 SquareChannel::GetFrequency() const {
  return nrx3 | (nrx4.period << 8);
}

void SquareChannel::SetFrequency(u16 freq) {
  nrx3 = freq & 0xff;
  nrx4.period = (freq >> 8) & 0b111;
}
