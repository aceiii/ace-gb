#pragma once

#include <string>
#include <string_view>
#include <unordered_map>


class ErrorMessages {
public:
  void AddError(std::string_view key, std::string_view message);
  void ClearError(std::string_view key);

  void Draw();

private:
  std::unordered_map<std::string, std::string> messages_ {};
};
