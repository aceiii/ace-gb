#include <utility>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "interrupt_device.h"

bool InterruptDevice::IsValidFor(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::IF):
    case std::to_underlying(IO::IE):
      return true;
    default: return false;
  }
}

void InterruptDevice::Write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::IF):
      flag.val = byte;
      return;
    case std::to_underlying(IO::IE):
      enable.val = byte;
      return;
    default: std::unreachable();
  }
}

uint8_t InterruptDevice::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::IF):
      return flag.val | 0b11100000;
    case std::to_underlying(IO::IE):
      return enable.val;
    default: std::unreachable();
  }
}

void InterruptDevice::Reset() {
  flag.reset();
  enable.reset();
}

void InterruptDevice::enable_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable.vblank = 1;
      return;
    case Interrupt::Stat:
      enable.lcd = 1;
      return;
    case Interrupt::Timer:
      enable.timer = 1;
      return;
    case Interrupt::Serial:
      enable.serial = 1;
      return;
    case Interrupt::Joypad:
      enable.joypad = 1;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::disable_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable.vblank = 0;
      return;
    case Interrupt::Stat:
      enable.lcd = 0;
      return;
    case Interrupt::Timer:
      enable.timer = 0;
      return;
    case Interrupt::Serial:
      enable.serial = 0;
      return;
    case Interrupt::Joypad:
      enable.joypad = 0;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::request_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag.vblank = 1;
      return;
    case Interrupt::Stat:
      flag.lcd = 1;
      return;
    case Interrupt::Timer:
      flag.timer = 1;
      return;
    case Interrupt::Serial:
      flag.serial = 1;
      return;
    case Interrupt::Joypad:
      flag.joypad = 1;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::clear_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag.vblank = 0;
      return;
    case Interrupt::Stat:
      flag.lcd = 0;
      return;
    case Interrupt::Timer:
      flag.timer = 0;
      return;
    case Interrupt::Serial:
      flag.serial = 0;
      return;
    case Interrupt::Joypad:
      flag.joypad = 0;
      return;
    default: std::unreachable();
  }
}

bool InterruptDevice::is_requested(Interrupt interrupt) const {
  switch (interrupt) {
    case Interrupt::VBlank:
      return enable.vblank & flag.vblank;
    case Interrupt::Stat:
      return enable.lcd & flag.lcd;
    case Interrupt::Timer:
      return enable.timer & flag.timer;
    case Interrupt::Serial:
      return enable.serial & flag.serial;
    case Interrupt::Joypad:
      return enable.joypad & flag.joypad;
    default: std::unreachable();
  }
}
