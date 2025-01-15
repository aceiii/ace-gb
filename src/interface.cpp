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

    DrawText(fmt::format("{}", emulator.registers()).c_str(), 20, 32, 12, RAYWHITE);
    DrawText(fmt::format("cycles={}", emulator.cycles()).c_str(), 20, 48, 12, RAYWHITE);
    DrawText(fmt::format("mode={}", magic_enum::enum_name(emulator.mode())).c_str(), 20, 64, 12, RAYWHITE);
    DrawText(fmt::format("stat={}", emulator.read8(std::to_underlying(IO::STAT))).c_str(), 20, 80, 12, RAYWHITE);

    Instruction instr = emulator.instr();
    DrawText(fmt::format("opcode={}, bytes={}, cycles={}/{}", magic_enum::enum_name(instr.opcode), instr.bytes, instr.cycles, instr.cycles_cond).c_str() , 20, 96, 12, RAYWHITE);

    uint8_t pc = emulator.registers().pc;
    DrawText(fmt::format("PC=0x{:02x}", pc).c_str() , 20, 128, 12, RAYWHITE);
    DrawText(fmt::format("@PC+0={:02x}", emulator.read8(pc)).c_str() , 20, 144, 12, RAYWHITE);
    DrawText(fmt::format("@PC+1={:02x}", emulator.read8(pc+1)).c_str() , 20, 160, 12, RAYWHITE);
    DrawText(fmt::format("@PC+2={:02x}", emulator.read8(pc+2)).c_str() , 20, 176, 12, RAYWHITE);
    DrawText(fmt::format("@PC+4={:02x}", emulator.read8(pc+3)).c_str() , 20, 192, 12, RAYWHITE);


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
      ImGui::Separator();
      if (ImGui::MenuItem("Step", nullptr, nullptr, !emulator.is_playing())) {
        emulator.step();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    rlImGuiEnd();

    EndDrawing();
  }

  spdlog::info("Shutting down...");
}
