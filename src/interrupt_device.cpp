#include <utility>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "interrupt_device.hpp"

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
      flag_.val = byte;
      return;
    case std::to_underlying(IO::IE):
      enable_.val = byte;
      return;
    default: std::unreachable();
  }
}

uint8_t InterruptDevice::Read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::IF):
      return flag_.val | 0b11100000;
    case std::to_underlying(IO::IE):
      return enable_.val;
    default: std::unreachable();
  }
}

void InterruptDevice::Reset() {
  flag_.reset();
  enable_.reset();
}

void InterruptDevice::EnableInterrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable_.vblank = 1;
      return;
    case Interrupt::Stat:
      enable_.lcd = 1;
      return;
    case Interrupt::Timer:
      enable_.timer = 1;
      return;
    case Interrupt::Serial:
      enable_.serial = 1;
      return;
    case Interrupt::Joypad:
      enable_.joypad = 1;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::DisableInterrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      enable_.vblank = 0;
      return;
    case Interrupt::Stat:
      enable_.lcd = 0;
      return;
    case Interrupt::Timer:
      enable_.timer = 0;
      return;
    case Interrupt::Serial:
      enable_.serial = 0;
      return;
    case Interrupt::Joypad:
      enable_.joypad = 0;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::RequestInterrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag_.vblank = 1;
      return;
    case Interrupt::Stat:
      flag_.lcd = 1;
      return;
    case Interrupt::Timer:
      flag_.timer = 1;
      return;
    case Interrupt::Serial:
      flag_.serial = 1;
      return;
    case Interrupt::Joypad:
      flag_.joypad = 1;
      return;
    default: std::unreachable();
  }
}

void InterruptDevice::ClearInterrupt(Interrupt interrupt) {
  switch (interrupt) {
    case Interrupt::VBlank:
      flag_.vblank = 0;
      return;
    case Interrupt::Stat:
      flag_.lcd = 0;
      return;
    case Interrupt::Timer:
      flag_.timer = 0;
      return;
    case Interrupt::Serial:
      flag_.serial = 0;
      return;
    case Interrupt::Joypad:
      flag_.joypad = 0;
      return;
    default: std::unreachable();
  }
}

bool InterruptDevice::IsInterruptRequested(Interrupt interrupt) const {
  switch (interrupt) {
    case Interrupt::VBlank:
      return enable_.vblank & flag_.vblank;
    case Interrupt::Stat:
      return enable_.lcd & flag_.lcd;
    case Interrupt::Timer:
      return enable_.timer & flag_.timer;
    case Interrupt::Serial:
      return enable_.serial & flag_.serial;
    case Interrupt::Joypad:
      return enable_.joypad & flag_.joypad;
    default: std::unreachable();
  }
}
