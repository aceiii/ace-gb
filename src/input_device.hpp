#pragma once

#include "types.hpp"
#include "mmu_device.hpp"
#include "joypad.hpp"
#include "interrupt_device.hpp"

struct InputRegister {
  union {
    struct {
      union {
        struct {
          u8 a_right: 1;
          u8 b_left: 1;
          u8 select_up: 1;
          u8 start_down: 1;
        };
        u8 buttons : 4;
      };
      union {
        struct {
          u8 sel_buttons: 1;
          u8 sel_dpad: 1;
        };
        u8 select : 2;
      };
      u8 unused: 2;
    };
    u8 val;
  };

  inline void reset() {
    val = 0xff;
  }
};

class InputDevice : public MmuDevice {
public:
  explicit InputDevice(InterruptDevice& interrupts);

  [[nodiscard]] bool IsValidFor(u16 addr) const override;
  void Write8(u16 addr, u8 byte) override;
  [[nodiscard]] u8 Read8(u16 addr) const override;
  void Reset() override;

  void Update(JoypadButton button, bool pressed);
  bool IsPressed(JoypadButton button) const;

private:
  InterruptDevice& interrupts_;

  InputRegister reg_buttons_;
  InputRegister reg_dpad_;
};
