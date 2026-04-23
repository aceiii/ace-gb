#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

#include "timer.hpp"
#include "cpu.hpp"

namespace {
  constexpr u8 kDivTimerBit[] = {
    9,
    3,
    5,
    7,
  };
}

void Timer::Init(TimerConfig cfg) {
  interrupts_ = cfg.interrupts;
}

bool Timer::IsValidFor(u16 addr) const {
  switch (addr) {
    case std::to_underlying(IO::DIV):
    case std::to_underlying(IO::TIMA):
    case std::to_underlying(IO::TMA):
    case std::to_underlying(IO::TAC):
      return true;
    default: return false;
  }
}

void Timer::Write8(u16 addr, u8 byte) {
  ZoneScoped;
  u16 prev_div = regs_.div;
  u8 prev_tac = regs_.tac;
  switch (addr) {
    case std::to_underlying(IO::DIV):
      regs_.div = 0;
      ComputeTimer(prev_div, prev_tac);
      return;
    case std::to_underlying(IO::TIMA):
      if (overflowing_ != 0) {
        regs_.tima = byte;
        overflowing_ = -1;
        ComputeTimer(prev_div, prev_tac);
      }
      return;
    case std::to_underlying(IO::TMA):
      regs_.tma = byte;
      if (overflowing_ == 0) {
        regs_.tima = byte;
      }
      return;
    case std::to_underlying(IO::TAC):
      regs_.tac = byte | 0b11111000;
      ComputeTimer(prev_div, prev_tac);
      return;
    default: std::unreachable();
  }
}

u8 Timer::Read8(u16 addr) const {
  ZoneScoped;
  switch (addr) {
    case std::to_underlying(IO::DIV):
      return div();
    case std::to_underlying(IO::TIMA):
      return regs_.tima;
    case std::to_underlying(IO::TMA):
      return regs_.tma;
    case std::to_underlying(IO::TAC):
      return regs_.tac;
    default: std::unreachable();
  }
}

void Timer::Reset() {
  regs_.div = 0;
  regs_.tima = 0;
  regs_.tma = 0;
  regs_.tac = 0;
  overflowing_ = false;
  cycles = 0;
}

void Timer::Execute(u8 cycles) {
  ZoneScoped;

  if (overflowing_ >= 0) {
    overflowing_ -= 1;
    if (overflowing_ == 0) {
      regs_.tima = regs_.tma;
      interrupts_->RequestInterrupt(Interrupt::Timer);
      overflowing_ = false;
    }
  }

  u16 prev_div = regs_.div;
  regs_.div += 4;
  ComputeTimer(prev_div, regs_.tac);
}

void Timer::ComputeTimer(u16 prev_div, u8 prev_tac) {
  u8 bit = kDivTimerBit[regs_.clock_select];
  u8 prev_bit = (prev_div >> kDivTimerBit[prev_tac & 0b11]) & (prev_tac >> 2) & 0b1;
  u8 enable_bit = (regs_.div >> kDivTimerBit[regs_.clock_select]) & regs_.enable_tima & 0b1;

  if (overflowing_ == 1) {
    return;
  }

  if (prev_bit && !enable_bit) {
    regs_.tima += 1;
    if (regs_.tima == 0) {
      overflowing_ = 1;
    }
  }
}

void Timer::OnTick(bool double_speed) {
  ZoneScoped;
  Execute(4);
  cycles += 4;
}

u16 Timer::div() const {
  return regs_.div >> 8;
}
