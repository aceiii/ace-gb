#pragma once

#include "mmu_device.hpp"
#include "joypad.hpp"
#include "interrupt_device.hpp"

struct InputRegister {
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

  [[nodiscard]] bool IsValidFor(uint16_t addr) const override;
  void Write8(uint16_t addr, uint8_t byte) override;
  [[nodiscard]] uint8_t Read8(uint16_t addr) const override;
  void Reset() override;

  void Update(JoypadButton button, bool pressed);
  bool IsPressed(JoypadButton button) const;

private:
  InterruptDevice &interrupts_;

  InputRegister reg_buttons_;
  InputRegister reg_dpad_;
};
