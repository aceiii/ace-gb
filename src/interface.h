#pragma once

#include <string>
#include <vector>

#include "config.h"
#include "emulator.h"


struct InterfaceSettings {
  std::vector<std::string> recent_files;
};

class Interface {
public:
  explicit Interface();
  ~Interface();

  void run();
  void load_cartridge();
  void load_cart_rom(const std::string &path);

private:
  Emulator emulator;
  Config<InterfaceSettings> config;
  bool auto_start = true;
  std::string error_message;

  void cleanup();

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
  void render_input(bool &show_window);
};
