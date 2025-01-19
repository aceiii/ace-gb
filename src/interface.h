#pragma once

#include <string>

#include "cpu.h"

class Interface {
public:
  explicit Interface();
  ~Interface();

  void run();
  void load_cartridge();

private:
  std::string error_message;
  void render_error();
};
