#pragma once

#include <imgui.h>
#include <string>

class AppLog {
public:
  AppLog();

  void AddLog(const std::string& string);
  void Clear();
  void Draw();

private:
  ImGuiTextBuffer buffer_;
  ImGuiTextFilter filter_;
  ImVector<int> line_offsets_;
  bool auto_scroll_;
};
