#include <spdlog/spdlog.h>

#include "serial_device.h"
#include "io.hpp"

SerialDevice::SerialDevice(InterruptDevice &interrupts):interrupts{interrupts} {
  sb = 0xff;
  sc.val = 0;
  sc.unused = 0x1f;
}

bool SerialDevice::IsValidFor(uint16_t addr) const {
  return addr == std::to_underlying(IO::SB) || addr == std::to_underlying(IO::SC);
}

void SerialDevice::Write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::SB): sb = byte; return;
    case std::to_underlying(IO::SC): {
      sc.val = byte;
      if (sc.transfer_enable == 0b1 && sc.clock_select == 0b1) {
        spdlog::info("start serial transfer");
        transfer_bytes = 8;
      }
      return;
    }
    default: std::unreachable();
  }
}

uint8_t SerialDevice::Read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::SB): return sb;
    case std::to_underlying(IO::SC): return sc.val | 0b01111110;
    default: std::unreachable();
  }
}

void SerialDevice::Reset() {
  sb = 0xff;
  sc.val = 0;
  sc.unused = 0x1f;
}

void SerialDevice::OnTick() {
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

  if (clock % 256 == 0) {
    return;
  }

  if (!transfer_bytes) {
    return;
  }

  spdlog::info("serial transfer -- bytes left: {}", transfer_bytes);

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
    interrupts.RequestInterrupt(Interrupt::Serial);
  }
}

void SerialDevice::trigger_callbacks() {
  std::string str = str_buffer;
  for (const auto &callback : callbacks) {
    callback(str);
  }
}
