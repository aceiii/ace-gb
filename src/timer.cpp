#include <spdlog/spdlog.h>

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

Timer::Timer(InterruptDevice &interrupts):interrupts{interrupts} {}

bool Timer::valid_for(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::DIV)-1:
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
    case std::to_underlying(IO::DIV)-1:
      return;
    case std::to_underlying(IO::DIV):
      regs.div = 0;
      return;
    case std::to_underlying(IO::TIMA):
      regs.tima = byte;
      return;
    case std::to_underlying(IO::TMA):
      regs.tma = byte;
      tima_counter = 0;
      return;
    case std::to_underlying(IO::TAC):
      regs.tac = (byte & 0b111) | (0b11111000);
      return;
    default: std::unreachable();
  }
}

uint8_t Timer::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::DIV)-1:
      return regs.div & 0xff;
    case std::to_underlying(IO::DIV):
      return regs.div >> 8;
    case std::to_underlying(IO::TIMA):
      return regs.tima;
    case std::to_underlying(IO::TMA):
      return regs.tma;
    case std::to_underlying(IO::TAC):
      return regs.tac;
    default: std::unreachable();
  }
}

void Timer::reset() {
  regs.div = 0;
  regs.tima = 0;
  regs.tma = 0;
  regs.tac = 0;
}

void Timer::execute(uint8_t cycles) {
  regs.div += cycles;

  if (regs.enable_tima) {
    tima_counter += cycles;

    const auto freq = kTimerFreqs[regs.clock_select];
    auto overflows = tima_counter / freq;
    tima_counter = tima_counter % freq;

    while (overflows) {
      regs.tima += 1;
      if (regs.tima == 0) {
        regs.tima = regs.tma;
        interrupts.request_interrupt(Interrupt::Timer);
      }
      overflows -= 1;
    }
  }
}

void Timer::on_tick() {
  execute(4);
}

