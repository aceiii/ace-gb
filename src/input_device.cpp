#include <utility>

#include "io.h"
#include "input_device.h"

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
  switch (button) {
    case JoypadButton::Start: reg_buttons.start_down = on_off; break;
    case JoypadButton::Select: reg_buttons.select_up = on_off; break;
    case JoypadButton::Up: reg_dpad.select_up = on_off; break;
    case JoypadButton::Down: reg_dpad.start_down = on_off; break;
    case JoypadButton::Left: reg_dpad.b_left = on_off; break;
    case JoypadButton::Right: reg_dpad.a_right = on_off; break;
    case JoypadButton::A: reg_buttons.a_right = on_off; break;
    case JoypadButton::B: reg_buttons.b_left = on_off; break;
    default: std::unreachable();
  }
}
