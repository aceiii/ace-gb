#include <spdlog/spdlog.h>

#include "serial_device.h"
#include "io.h"

SerialDevice::SerialDevice(InterruptDevice &interrupts):interrupts{interrupts} {
  sb = 0;
  sc.val = 0;
  sc.unused = 0x1f;
}

bool SerialDevice::valid_for(uint16_t addr) const {
  return addr == std::to_underlying(IO::SB) || addr == std::to_underlying(IO::SC);
}

void SerialDevice::write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::SB): sb = byte; return;
    case std::to_underlying(IO::SC): {
      sc.val = byte;
      if (sc.transfer_enable) {
        transfer_bytes = 8;
      }
      return;
    }
    default: std::unreachable();
  }
}

uint8_t SerialDevice::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::SB): return sb;
    case std::to_underlying(IO::SC): return sc.val & 0b01111110;
    default: std::unreachable();
  }
}

void SerialDevice::reset() {
  sb = 0;
  sc.val = 0;
  sc.unused = 0x1f;
}

void SerialDevice::on_tick() {
  step();
}

std::string_view SerialDevice::line_buffer() const {
  return str_buffer;
}

void SerialDevice::on_line(const LineCallback& callback) {
  callbacks.push_back(callback);
}

void SerialDevice::step() {
  clock += 4;

  if (!sc.transfer_enable) {
    return;
  }

  if (clock != 256) {
    return;
  }

  clock = 0;

  byte_buffer = (byte_buffer << 1) | ((sb & 0x80) >> 7);
  sb <<= 1;
  sb |= 0b1;

  transfer_bytes--;
  if (transfer_bytes == 0) {
    char c = static_cast<char>(byte_buffer);
    if (c == '\n') {
      trigger_callbacks();
      str_buffer.clear();
    } else {
      str_buffer += c;
      if (str_buffer.length() >= 80) {
        trigger_callbacks();
        str_buffer.clear();
      }
    }

    sc.transfer_enable = 0;
    interrupts.request_interrupt(Interrupt::Serial);
  }
}

void SerialDevice::trigger_callbacks() {
  std::string str = str_buffer;
  for (const auto &callback : callbacks) {
    callback(str);
  }
}
