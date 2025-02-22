#pragma once

#include "mmu_device.h"
#include "joypad.h"
#include "interrupt_device.h"

struct input_register {
  union {
    struct {
      union {
        struct {
          uint8_t a_right: 1;
          uint8_t b_left: 1;
          uint8_t select_up: 1;
          uint8_t start_down: 1;
        };
        uint8_t buttons : 4;
      };
      union {
        struct {
          uint8_t sel_buttons: 1;
          uint8_t sel_dpad: 1;
        };
        uint8_t select : 2;
      };
      uint8_t unused: 2;
    };
    uint8_t val;
  };

  inline void reset() {
    val = 0xff;
  }
};

class InputDevice : public MmuDevice {
public:
  explicit InputDevice(InterruptDevice &interrupts);

  [[nodiscard]] bool valid_for(uint16_t addr) const override;
  void write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t read8(uint16_t addr) const override;
  void reset() override;

  void update(JoypadButton button, bool pressed);

private:
  InterruptDevice &interrupts;

  input_register reg_buttons;
  input_register reg_dpad;
};
