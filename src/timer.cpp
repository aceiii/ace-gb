#include <spdlog/spdlog.h>

#include "timer.hpp"
#include "cpu.hpp"

namespace {
  constexpr uint16_t kTimerFreqs[] = {
    kClockSpeed / 4096,
    kClockSpeed / 262144,
    kClockSpeed / 65535,
    kClockSpeed / 16384,
  };
}

Timer::Timer(InterruptDevice &interrupts):interrupts_{interrupts} {}

bool Timer::IsValidFor(uint16_t addr) const {
  switch (addr) {
//    case std::to_underlying(IO::DIV)-1:
    case std::to_underlying(IO::DIV):
    case std::to_underlying(IO::TIMA):
    case std::to_underlying(IO::TMA):
    case std::to_underlying(IO::TAC):
      return true;
    default: return false;
  }
}

void Timer::Write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
//    case std::to_underlying(IO::DIV)-1:
//      return;
    case std::to_underlying(IO::DIV):
      regs_.div = 0;
      return;
    case std::to_underlying(IO::TIMA):
      regs_.tima = byte;
      return;
    case std::to_underlying(IO::TMA):
      regs_.tma = byte;
      tima_counter_ = 0;
      return;
    case std::to_underlying(IO::TAC):
      regs_.tac = (byte & 0b111) | (0b11111000);
      return;
    default: std::unreachable();
  }
}

uint8_t Timer::Read8(uint16_t addr) const {
  switch (addr) {
//    case std::to_underlying(IO::DIV)-1:
//      return regs.div & 0xff;
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
}

void Timer::Execute(uint8_t cycles) {
  regs_.div += cycles;

  if (regs_.enable_tima) {
    tima_counter_ += cycles;

    const auto freq = kTimerFreqs[regs_.clock_select];
    auto overflows = tima_counter_ / freq;
    tima_counter_ = tima_counter_ % freq;

    while (overflows) {
      regs_.tima += 1;
      if (regs_.tima == 0) {
        regs_.tima = regs_.tma;
        interrupts_.RequestInterrupt(Interrupt::Timer);
      }
      overflows -= 1;
    }
  }
}

void Timer::OnTick() {
  Execute(4);
}

uint16_t Timer::div() const {
  return regs_.div >> 8;
}
