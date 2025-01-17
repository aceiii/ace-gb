#pragma once

#include <string>

#include "emulator.h"

class Interface {
public:
  explicit Interface();
  ~Interface();

  void run();
  void load_cartridge();

private:
  Emulator emulator;

  bool auto_start = true;
  std::string error_message;
  void render_error();
  void render_info();
};
