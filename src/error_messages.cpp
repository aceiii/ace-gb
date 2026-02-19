#include <raylib.h>

#include "error_messages.hpp"


void ErrorMessages::AddError(std::string_view key, std::string_view message) {
  messages_.insert_or_assign(std::string{key}, std::string{message});
}

void ErrorMessages::ClearError(std::string_view key) {
  messages_.erase(std::string{key});
}

void ErrorMessages::Draw() {
  if (messages_.empty()) {
    return;
  }

  const int padding = 10;
  const int spacing = 2;
  const int text_height = 20;
  const int screen_width = GetScreenWidth();
  const int screen_height = GetScreenHeight();
  int offset_y = (screen_height / 2) - (((text_height + spacing + spacing + padding + padding) * messages_.size()) / 2);

  for (const auto& pair : messages_) {
    auto& error_message = pair.second;
    auto text_width = MeasureText(error_message.c_str(), 15);
    auto text_x = (screen_width - text_width) / 2;
    auto text_y = offset_y + spacing + padding;

    DrawRectangle(text_x - padding, text_y - padding, text_width + padding + padding, text_height + padding + padding, Color { 32, 32, 32, 127 });
    DrawText(error_message.c_str(), text_x, text_y, 15, RED);

    offset_y += text_height + spacing + spacing + padding + padding;
  }
}
