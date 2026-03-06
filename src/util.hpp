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
  ImColor col(color.r, color.g, color.b, color.a);
  return col.Value;
}

Color ImVec4ToColor(const ImVec4& vec) {
  ImU32 col = ImColor(vec);
  return GetColor(col);
}
