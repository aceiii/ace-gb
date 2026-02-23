#include "applog.hpp"


AppLog::AppLog() : auto_scroll_{true} {
  Clear();
}

void AppLog::AddLog(std::string_view log) {
  int old_size = buffer_.size();
  buffer_.append(std::to_address(log.begin()), std::to_address(log.end()));
  for (int new_size = buffer_.size(); old_size < new_size; old_size += 1) {
    if (buffer_[old_size] == '\n') {
      line_offsets_.push_back(old_size + 1);
    }
  }
}

void AppLog::Clear() {
  buffer_.clear();
  line_offsets_.clear();
  line_offsets_.push_back(0);
}

void AppLog::Draw() {
  if (ImGui::BeginPopup("Options")) {
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);
    ImGui::EndPopup();
  }

  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("Options");
  }

  ImGui::SameLine();
  bool should_clear = ImGui::Button("Clear");
  ImGui::SameLine();
  bool did_copy = ImGui::Button("Copy");
  ImGui::SameLine();
  filter_.Draw("Filter", -100.0f);

  ImGui::Separator();

  if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    if (should_clear) {
      Clear();
    }

    if (did_copy) {
      ImGui::LogToClipboard();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const char* buf = std::to_address(buffer_.begin());
    const char* buf_end = std::to_address(buffer_.end());
    if (filter_.IsActive()) {
      for (int line_no = 0; line_no < line_offsets_.Size; line_no++) {
        const char* line_start = buf + line_offsets_[line_no];
        const char* line_end = (line_no + 1 < line_offsets_.Size)
                                   ? (buf + line_offsets_[line_no + 1] - 1)
                                   : buf_end;
        if (filter_.PassFilter(line_start, line_end)) {
          ImGui::TextUnformatted(line_start, line_end);
        }
      }
    } else {
      ImGuiListClipper clipper;
      clipper.Begin(line_offsets_.Size);
      while (clipper.Step()) {
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
          const char* line_start = buf + line_offsets_[line_no];
          const char* line_end = (line_no + 1 < line_offsets_.Size)
                                    ? (buf + line_offsets_[line_no + 1] - 1)
                                    : buf_end;
          ImGui::TextUnformatted(line_start, line_end);
        }
      }
      clipper.End();
    }
    ImGui::PopStyleVar();

    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
      ImGui::SetScrollHereY(1.0f);
    }
  }
  ImGui::EndChild();
};
