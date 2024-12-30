#include "interface.h"
#include "emulator.h"

#include <raylib.h>
#include <imgui.h>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>

namespace {
  const int kDefaulWindowWidth = 800;
  const int kDefaultWindowHeight = 600;
  const char* kWindowTitle = "Ace::GB - GameBoy Emulator";

  Emulator emulator;
}

Interface::Interface() {
  spdlog::info("Initializing interface");

  NFD_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(kDefaulWindowWidth, kDefaultWindowHeight, kWindowTitle);
  InitAudioDevice();

  int monitor = GetCurrentMonitor();
  spdlog::trace("Current monitor: {}", monitor);

  int monitor_width = GetMonitorWidth(monitor);
  int monitor_height = GetMonitorHeight(monitor);
  spdlog::trace("Monitor resolution: {}x{}", monitor_width, monitor_height);

  SetExitKey(KEY_NULL);
  rlImGuiSetup(true);

  while (!IsWindowReady()) {
    // pass
  }
}

Interface::~Interface() {
  spdlog::info("Cleaning up interface");
}

void Interface::run() {
  spdlog::info("Running...");

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    rlImGuiBegin();

    rlImGuiEnd();


    DrawFPS(10, GetScreenHeight() - 24);

    EndDrawing();
  }

  spdlog::info("Shutting down...");
}
