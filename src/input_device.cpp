#include <utility>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "io.hpp"
#include "input_device.hpp"

InputDevice::InputDevice(InterruptDevice &interrupts): interrupts_{interrupts} {
}

[[nodiscard]] bool InputDevice::IsValidFor(uint16_t addr) const {
  return addr == std::to_underlying(IO::P1);
}

void InputDevice::Write8(uint16_t addr, uint8_t byte) {
  reg_buttons_.select = (byte >> 4) & 0b11;
}

[[nodiscard]] uint8_t InputDevice::Read8(uint16_t addr) const {
  uint8_t buttons = 0;
  if (reg_buttons_.sel_buttons) {
    buttons |= ~reg_buttons_.buttons;
  }
  if (reg_buttons_.sel_dpad) {
    buttons |= ~reg_dpad_.buttons;
  }

  return (reg_buttons_.select << 4) | (~buttons & 0b1111) | (0b11000000);
}

void InputDevice::Reset() {
  reg_buttons_.reset();
  reg_dpad_.reset();
}

void InputDevice::Update(JoypadButton button, bool pressed) {
  uint8_t on_off = pressed ? 0 : 1;
  bool flipped;

  switch (button) {
    case JoypadButton::Start:
      flipped = !on_off && reg_buttons_.start_down;
      reg_buttons_.start_down = on_off;
      break;
    case JoypadButton::Select:
      flipped = !on_off && reg_buttons_.select_up;
      reg_buttons_.select_up = on_off;
      break;
    case JoypadButton::Up:
      flipped = !on_off && reg_dpad_.select_up;
      reg_dpad_.select_up = on_off;
      break;
    case JoypadButton::Down:
      flipped = !on_off && reg_dpad_.start_down;
      reg_dpad_.start_down = on_off;
      break;
    case JoypadButton::Left:
      flipped = !on_off && reg_dpad_.b_left;
      reg_dpad_.b_left = on_off;
      break;
    case JoypadButton::Right:
      flipped = !on_off && reg_dpad_.a_right;
      reg_dpad_.a_right = on_off;
      break;
    case JoypadButton::A:
      flipped = !on_off && reg_buttons_.a_right;
      reg_buttons_.a_right = on_off;
      break;
    case JoypadButton::B:
      flipped = !on_off && reg_buttons_.b_left;
      reg_buttons_.b_left = on_off;
      break;
    default: std::unreachable();
  }

  if (flipped) {
    interrupts_.RequestInterrupt(Interrupt::Joypad);
  }
}

bool InputDevice::IsPressed(JoypadButton button) const {
  switch (button) {
    case JoypadButton::Start: return !reg_buttons_.start_down;
    case JoypadButton::Select: return !reg_buttons_.select_up;
    case JoypadButton::Up: return !reg_dpad_.select_up;
    case JoypadButton::Down: return !reg_dpad_.start_down;
    case JoypadButton::Left: return !reg_dpad_.b_left;
    case JoypadButton::Right: return !reg_dpad_.a_right;
    case JoypadButton::A: return !reg_buttons_.a_right;
    case JoypadButton::B: return !reg_buttons_.b_left;
    default: std::unreachable();
  }
}
