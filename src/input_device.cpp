#include <utility>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "io.h"
#include "input_device.h"

InputDevice::InputDevice(InterruptDevice &interrupts): interrupts{interrupts} {
}

[[nodiscard]] bool InputDevice::valid_for(uint16_t addr) const {
  return addr == std::to_underlying(IO::P1);
}

void InputDevice::write8(uint16_t addr, uint8_t byte) {
  reg_buttons.select = (byte >> 4) & 0b11;
}

[[nodiscard]] uint8_t InputDevice::read8(uint16_t addr) const {
  uint8_t buttons = 0;
  if (reg_buttons.sel_buttons) {
    buttons |= ~reg_buttons.buttons;
  }
  if (reg_buttons.sel_dpad) {
    buttons |= ~reg_dpad.buttons;
  }

  return (reg_buttons.select << 4) | (~buttons & 0b1111) | (0b11000000);
}

void InputDevice::reset() {
  reg_buttons.reset();
  reg_dpad.reset();
}

void InputDevice::update(JoypadButton button, bool pressed) {
  uint8_t on_off = pressed ? 0 : 1;
  bool flipped;

  switch (button) {
    case JoypadButton::Start:
      flipped = !on_off && reg_buttons.start_down;
      reg_buttons.start_down = on_off;
      break;
    case JoypadButton::Select:
      flipped = !on_off && reg_buttons.select_up;
      reg_buttons.select_up = on_off;
      break;
    case JoypadButton::Up:
      flipped = !on_off && reg_dpad.select_up;
      reg_dpad.select_up = on_off;
      break;
    case JoypadButton::Down:
      flipped = !on_off && reg_dpad.start_down;
      reg_dpad.start_down = on_off;
      break;
    case JoypadButton::Left:
      flipped = !on_off && reg_dpad.b_left;
      reg_dpad.b_left = on_off;
      break;
    case JoypadButton::Right:
      flipped = !on_off && reg_dpad.a_right;
      reg_dpad.a_right = on_off;
      break;
    case JoypadButton::A:
      flipped = !on_off && reg_buttons.a_right;
      reg_buttons.a_right = on_off;
      break;
    case JoypadButton::B:
      flipped = !on_off && reg_buttons.b_left;
      reg_buttons.b_left = on_off;
      break;
    default: std::unreachable();
  }

  if (flipped) {
    interrupts.request_interrupt(Interrupt::Joypad);
  }
}

bool InputDevice::is_pressed(JoypadButton button) const {
  switch (button) {
    case JoypadButton::Start: return !reg_buttons.start_down;
    case JoypadButton::Select: return !reg_buttons.select_up;
    case JoypadButton::Up: return !reg_dpad.select_up;
    case JoypadButton::Down: return !reg_dpad.start_down;
    case JoypadButton::Left: return !reg_dpad.b_left;
    case JoypadButton::Right: return !reg_dpad.a_right;
    case JoypadButton::A: return !reg_buttons.a_right;
    case JoypadButton::B: return !reg_buttons.b_left;
    default: std::unreachable();
  }
}
