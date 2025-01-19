#include <utility>

#include "interrupt_device.h"

bool InterruptDevice::valid_for(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::IF):
    case std::to_underlying(IO::IE):
      return true;
    default: return false;
  }
}

void InterruptDevice::write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::IF):
      flag.val = byte;
      return;
    case std::to_underlying(IO::IE):
      enable.val = byte;
      return;
    default: return;
  }
}

uint8_t InterruptDevice::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::IF):
      return flag.val;
    case std::to_underlying(IO::IE):
      return enable.val;
    default: return 0;
  }
}

void InterruptDevice::reset() {
  flag.reset();
  enable.reset();
}

void InterruptDevice::enable_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable.vblank = 1;
      return;
    case Interrupt::LCD:
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
    default: return;
  }
}

void InterruptDevice::disable_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable.vblank = 0;
      return;
    case Interrupt::LCD:
      enable.lcd = 0;
      return;
    case Interrupt::Timer:
      enable.timer = 0;
      return;
    case Interrupt::Serial:
      enable.serial =0 ;
      return;
    case Interrupt::Joypad:
      enable.joypad = 0;
      return;
    default: return;
  }
}

void InterruptDevice::request_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag.vblank = 1;
      return;
    case Interrupt::LCD:
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
    default: return;
  }
}

void InterruptDevice::clear_interrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag.vblank = 0;
      return;
    case Interrupt::LCD:
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
    default: return;
  }
}

bool InterruptDevice::is_requested(Interrupt interrupt) const {
  switch (interrupt) {
    case Interrupt::VBlank:
      return enable.vblank & flag.vblank;
    case Interrupt::LCD:
      return enable.lcd & flag.lcd;
    case Interrupt::Timer:
      return enable.timer & flag.timer;
    case Interrupt::Serial:
      return enable.serial & flag.serial;
    case Interrupt::Joypad:
      return enable.joypad & flag.joypad;
    default: return false;
  }
}
