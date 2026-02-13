#pragma once

#include <string>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include "args.h"
#include "config.h"
#include "emulator.h"
#include "recent_files.hpp"


namespace app {

constexpr size_t kMaxRecentFiles = 10;

struct InterfaceSettings {
  int screen_width;
  int screen_height;
  int screen_x;
  int screen_y;
  RecentFiles recent_files{kMaxRecentFiles};

  // Emulator
  bool auto_start;
  bool skip_boot_rom;
  std::string boot_rom_path;

  // Hardware
  bool show_lcd;
  bool show_tiles;
  bool show_tilemap1;
  bool show_tilemap2;
  bool show_sprites;
  bool show_cpu_registers;
  bool show_input;
  bool show_memory;

  bool enable_audio;
  bool enable_ch1;
  bool enable_ch2;
  bool enable_ch3;
  bool enable_ch4;
  float master_volume;
};

class Interface {
public:
  explicit Interface(Args args);
  ~Interface();

  void run();
  void load_cartridge();
  void load_cart_rom(const std::string &path);

private:
  Args args;
  Emulator emulator;
  Config<InterfaceSettings> config;
  std::string error_message;

  MemoryEditor mem_editor;

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

}
