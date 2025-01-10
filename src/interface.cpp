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

  emulator.init();

  while (!IsWindowReady()) {
    // pass
  }
}

Interface::~Interface() {
  spdlog::info("Cleaning up interface");

  emulator.cleanup();

  CloseAudioDevice();
  CloseWindow();
}

void Interface::run() {
  spdlog::info("Running...");

  while (!WindowShouldClose()) {
    emulator.update();

    BeginDrawing();
    ClearBackground(DARKGRAY);

    DrawFPS(10, GetScreenHeight() - 24);

    DrawText(fmt::format("{}", emulator.registers()).c_str(), 20, 32, 15, RAYWHITE);
    DrawText(fmt::format("cycles={}", emulator.cycles()).c_str(), 20, 48, 15, RAYWHITE);

    rlImGuiBegin();

    emulator.render();

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Emulator")) {
      if (ImGui::MenuItem("Play", nullptr, nullptr, !emulator.is_playing())) {
        emulator.play();
      }
      if (ImGui::MenuItem("Stop", nullptr, nullptr, emulator.is_playing())) {
        emulator.stop();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    rlImGuiEnd();

    EndDrawing();
  }

  spdlog::info("Shutting down...");
}
