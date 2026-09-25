#pragma once

#include <format>
#include <sstream>
#include <string>
#include <string_view>

#include "types.hpp"


struct Colour {
  u8 red = 0x00;
  u8 green = 0x00;
  u8 blue = 0x00;
  u8 alpha = 0xFF;

  std::string ToString() const {
    return std::format("{:02X}{:02X}{:02X}{:02X}", red, green, blue, alpha);
  }

  static Colour Parse(std::string_view str) {
    unsigned int val;
    std::string s{str};
    std::stringstream ss{s};
    ss >> std::hex >> val;

    Colour colour{
      .red = static_cast<u8>(val >> 24),
      .green = static_cast<u8>(val >> 16),
      .blue = static_cast<u8>(val >> 8),
      .alpha = static_cast<u8>(val),
    };

    return colour;
  }
};
