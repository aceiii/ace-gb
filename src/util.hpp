#pragma once

#include <format>
#include <sstream>
#include <string>
#include <raylib.h>
#include <imgui.h>


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

Color ImVec4ToColor(const ImVec4& vec) {
  return Color{
    .r = static_cast<unsigned char>(vec.x * 255.0f),
    .g = static_cast<unsigned char>(vec.y * 255.0f),
    .b = static_cast<unsigned char>(vec.z * 255.0f),
    .a = static_cast<unsigned char>(vec.w * 255.0f),
  };
}
