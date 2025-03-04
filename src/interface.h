#pragma once

#include <string>

#include "emulator.h"

class Interface {
public:
  explicit Interface();
  ~Interface();

  void run();
  void load_cartridge();
  void load_cart_rom(const std::string &path);

private:
  Emulator emulator;
  bool auto_start = true;
  std::string error_message;

  void play();
  void stop();
  void step();
  void reset();

  void render_error();
  void render_lcd(bool &show_window);
  void render_tiles(bool &show_window);
  void render_tilemap1(bool &show_window);
  void render_tilemap2(bool &show_window);
  void render_sprites(bool &show_window);
  void render_registers(bool &show_window);
};
