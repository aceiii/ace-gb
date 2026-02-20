#pragma once

#include <imgui.h>
#include <string>
#include <string_view>

class AppLog {
public:
  AppLog();

  void AddLog(std::string_view string);
  void Clear();
  void Draw();

private:
  ImGuiTextBuffer buffer_;
  ImGuiTextFilter filter_;
  ImVector<int> line_offsets_;
  bool auto_scroll_;
};
