#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

#include "serial_device.hpp"
#include "io.hpp"

SerialDevice::SerialDevice(InterruptDevice& interrupts):interrupts_{interrupts} {
  sb_ = 0xff;
  sc_.val = 0;
  sc_.unused = 0x1f;
}

bool SerialDevice::IsValidFor(uint16_t addr) const {
  return addr == std::to_underlying(IO::SB) || addr == std::to_underlying(IO::SC);
}

void SerialDevice::Write8(uint16_t addr, uint8_t byte) {
  switch (addr) {
    case std::to_underlying(IO::SB): sb_ = byte; return;
    case std::to_underlying(IO::SC): {
      sc_.val = byte;
      if (sc_.transfer_enable == 0b1 && sc_.clock_select == 0b1) {
        spdlog::info("start serial transfer");
        transfer_bytes_ = 8;
      }
      return;
    }
    default: std::unreachable();
  }
}

uint8_t SerialDevice::Read8(uint16_t addr) const {
  switch (addr) {
    case std::to_underlying(IO::SB): return sb_;
    case std::to_underlying(IO::SC): return sc_.val | 0b01111110;
    default: std::unreachable();
  }
}

void SerialDevice::Reset() {
  sb_ = 0xff;
  sc_.val = 0;
  sc_.unused = 0x1f;
}

void SerialDevice::OnTick() {
  ZoneScoped;
  Step();
}

std::string_view SerialDevice::LineBuffer() const {
  return str_buffer_;
}

void SerialDevice::OnLine(const LineCallback& callback) {
  callbacks_.push_back(callback);
}

void SerialDevice::Step() {
  ZoneScoped;

  clock_ += 4;

  if (!sc_.transfer_enable) {
    return;
  }

  if (clock_ % 256 == 0) {
    return;
  }

  if (!transfer_bytes_) {
    return;
  }

  spdlog::info("serial transfer -- bytes left: {}", transfer_bytes_);

  byte_buffer_ = (byte_buffer_ << 1) | ((sb_ & 0x80) >> 7);
  sb_ <<= 1;
  sb_ |= 0b1;

  transfer_bytes_--;
  if (transfer_bytes_ == 0) {
    char c = static_cast<char>(byte_buffer_);
    if (c == '\n') {
      TriggerCallbacks();
      str_buffer_.clear();
    } else {
      str_buffer_ += c;
      if (str_buffer_.length() >= 80) {
        TriggerCallbacks();
        str_buffer_.clear();
      }
    }

    sc_.transfer_enable = 0;
    interrupts_.RequestInterrupt(Interrupt::Serial);
  }
}

void SerialDevice::TriggerCallbacks() {
  std::string str = str_buffer_;
  for (const auto& callback : callbacks_) {
    callback(str);
  }
}
