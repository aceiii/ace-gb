#include <utility>

#include "io.h"
#include "input_device.h"

[[nodiscard]] bool InputDevice::valid_for(uint16_t addr) const {
  return addr == std::to_underlying(IO::P1);
}

void InputDevice::write8(uint16_t addr, uint8_t byte) {}

[[nodiscard]] uint8_t InputDevice::read8(uint16_t addr) const {
  return 0xff;
}

void InputDevice::reset() {}
