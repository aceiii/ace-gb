#pragma once

#include <format>
#include <sstream>
#include <string>
#include <raylib.h>
#include <imgui.h>

#include "types.hpp"


Color StringToColor(const std::string& str) {
  unsigned int val;
  std::stringstream ss{str};
  ss >> std::hex >> val;
  return GetColor(val);
}

std::string ColorToString(const Color& color) {
  const unsigned int val = ColorToInt(color);
  return std::format("{:08x}", val);
}

ImVec4 ColorToImVec4(const Color& color) {
  return ImVec4(
    color.r / 255.0f,
    color.g / 255.0f,
    color.b / 255.0f,
    color.a / 255.0f
  );
}

ImVec4 ColourToImVec4(const Colour& color) {
  return ImVec4(
    color.red / 255.0f,
    color.green / 255.0f,
    color.blue / 255.0f,
    color.alpha / 255.0f
  );
}

Color ImVec4ToColor(const ImVec4& vec) {
  return Color{
    .r = static_cast<u8>(vec.x * 255.0f),
    .g = static_cast<u8>(vec.y * 255.0f),
    .b = static_cast<u8>(vec.z * 255.0f),
    .a = static_cast<u8>(vec.w * 255.0f),
  };
}

Colour ImVec4ToColour(const ImVec4& vec) {
  return Colour{
    .red = static_cast<u8>(vec.x * 255.0f),
    .green = static_cast<u8>(vec.y * 255.0f),
    .blue = static_cast<u8>(vec.z * 255.0f),
    .alpha = static_cast<u8>(vec.w * 255.0f),
  };
}
