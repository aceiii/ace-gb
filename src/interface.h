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
  void render_tiles(bool &show_window);
  void render_tilemap1(bool &show_window);
  void render_tilemap2(bool &show_window);
};
