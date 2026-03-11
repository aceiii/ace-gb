#pragma once

#include <array>
#include <string>
#include <string_view>
#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include "applog.hpp"
#include "args.hpp"
#include "assembly_viewer.hpp"
#include "config.hpp"
#include "emulator.hpp"
#include "error_messages.hpp"
#include "recent_files.hpp"


namespace app {

constexpr size_t kDmgClockSpeed = 4194304;
constexpr size_t kGbcClockSpeed = 8388608;
constexpr float kFrameRate = 59.73;

constexpr size_t kMaxRecentFiles = 10;

struct InterfaceSettings {
  int screen_width;
  int screen_height;
  int screen_x;
  int screen_y;
  RecentFiles recent_files{kMaxRecentFiles};
  bool reset_view;

  // Emulator
  bool auto_start;
  bool skip_boot_rom;
  std::string dmg_boot_rom_path;
  std::string cgb_boot_rom_path;

  // Hardware
  bool show_lcd;
  bool show_tiles;
  bool show_tilemap1;
  bool show_tilemap2;
  bool show_sprites;
  bool show_palettes;
  bool show_cpu_registers;
  bool show_input;
  bool show_memory;
  bool show_instructions;
  bool show_logs;
  bool show_debugger;
  bool show_graphic_options;

  bool enable_audio;
  bool enable_ch1;
  bool enable_ch2;
  bool enable_ch3;
  bool enable_ch4;
  float master_volume;

  bool lock_framerate;
  bool show_scanlines;

  std::array<Color, 4> palette;
};

class Interface {
public:
  void Init(Args args);
  void Run();
  void Cleanup();

  void LoadCartridge();
  void LoadCartridgeCallback(std::string_view);
  void LoadCartRom(std::string_view path);
  void UnloadCartridge();

private:
  void SaveSettings();

  void Play();
  void Stop();
  void Step();
  void StepFrame();
  void Reset();

  void Update();
  void ConfigureDockSpace();
  void RenderError();
  void RenderDebugger();
  void RenderLCD();
  void RenderTiles();
  void RenderTilemap1();
  void RenderTilemap2();
  void RenderSprites();
  void RenderPalettes();
  void RenderRegisters();
  void RenderInput();
  void RenderMemory();
  void RenderInstructions();
  void RenderLogs();
  void RenderGraphicOptions();
  void RenderMainMenu();
  void RenderSettingsPopup();
  void RenderStatusBar();
  void ResetView();

  Args args_ {};
  Emulator emulator_ {};
  AssemblyViewer assembly_viewer_ {};
  Config<InterfaceSettings> config_ {};
  MemoryEditor mem_editor_ {};
  AppLog app_log_ {};
  ErrorMessages error_messages_ {};

  bool should_close_ = false;
  bool show_settings_ = false;
  bool init_dock_ = false;
};

}
