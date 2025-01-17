#include "timer.h"
#include "cpu.h"

namespace {
  constexpr uint16_t kTimerFreqs[] = {
    kClockSpeed / 4096,
    kClockSpeed / 262144,
    kClockSpeed / 65535,
    kClockSpeed / 16384,
  };
}

bool Timer::valid_for(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::DIV):
    case std::to_underlying(IO::TIMA):
    case std::to_underlying(IO::TMA):
    case std::to_underlying(IO::TAC):
      return true;
    default: return false;
  }
}

void Timer::write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::DIV):
      regs.div = 0;
      return;
    case std::to_underlying(IO::TIMA):
      regs.tima = byte;
      return;
    case std::to_underlying(IO::TMA):
      regs.tma = byte;
      return;
    case std::to_underlying(IO::TAC):
      regs.tac = byte;
      return;
    default: return;
  }
}

uint8_t Timer::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::DIV):
      return regs.div;
    case std::to_underlying(IO::TIMA):
      return regs.tima;
    case std::to_underlying(IO::TMA):
      return regs.tma;
    case std::to_underlying(IO::TAC):
      return regs.tac;
    default: return 0;
  }
}

void Timer::reset() {
  regs.div = 0;
  regs.tima = 0;
  regs.tma = 0;
  regs.tac = 0;
}

void Timer::execute(uint8_t cycles) {
  div_counter += cycles;
  if (div_counter >= 256) {
    div_counter -= 256;
    regs.div += 1;
  }

  if (regs.enable_tima) {
    tima_counter += cycles;

    auto freq = kTimerFreqs[regs.clock_select];
    if (tima_counter >= freq) {
      if (regs.tima == 255) {
        regs.tima = regs.tma;
        // TODO: overflowed, enable timer interrupt
      } else {
        regs.tima += 1;
      }
      tima_counter -= freq;
    }
  }
}

