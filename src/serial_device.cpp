#include <spdlog/spdlog.h>

#include "serial_device.h"
#include "io.h"

SerialDevice::SerialDevice(InterruptDevice &interrupts):interrupts{interrupts} {}

[[nodiscard]] bool SerialDevice::valid_for(uint16_t addr) const {
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

[[nodiscard]] uint8_t SerialDevice::read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::SB): return sb;
    case std::to_underlying(IO::SC): return sc.val;
    default: std::unreachable();
  }
}

void SerialDevice::reset() {
  sb = 0;
  sc.val = 0;
}

void SerialDevice::execute(uint8_t cycles) {
  auto step = [&]() {
    transfer_bytes--;
    byte_buffer = (byte_buffer << 1) | ((sb & 0x80) >> 7);
    sb <<= 1;

    if (!transfer_bytes) {
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
  };

  while (transfer_bytes && cycles) {
    step();
    cycles--;
  }
}

std::string_view SerialDevice::line_buffer() const {
  return str_buffer;
}

void SerialDevice::on_line(const LineCallback& callback) {
  callbacks.push_back(callback);
}

void SerialDevice::trigger_callbacks() {
  std::string str = str_buffer;
  for (const auto &callback : callbacks) {
    callback(str);
  }
}
