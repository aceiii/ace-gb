#pragma once

#include <string>

#include "cpu.h"

class Interface {
public:
  explicit Interface();
  ~Interface();

  void run();

private:
  std::string error_message;
  void render_error();
};
