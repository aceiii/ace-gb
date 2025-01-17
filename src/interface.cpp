#include <filesystem>
#include <string>
#include <raylib.h>
#include <imgui.h>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>

#include "interface.h"
#include "emulator.h"
#include "file.h"

namespace fs = std::filesystem;

namespace {
  constexpr int kDefaulWindowWidth = 800;
  constexpr int kDefaultWindowHeight = 600;
  constexpr char const* kWindowTitle = "Ace::GB - GameBoy Emulator";
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

  if (auto result = emulator.init(); !result) {
    spdlog::error("Failed to initialize emulator");
    error_message = result.error();
  }

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
  emulator.reset();

  spdlog::info("Running...");

  static bool should_close = false;

  while (!should_close) {
    if (WindowShouldClose()) {
      should_close = true;
    }

    emulator.update();

    BeginDrawing();
    ClearBackground(DARKGRAY);

    DrawFPS(10, GetScreenHeight() - 24);

    render_info();

    rlImGuiBegin();

    emulator.render();

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load Cartridge")) {
        spdlog::info("Loading cart...");
        load_cartridge();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {
        spdlog::info("Exiting...");
        should_close = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Emulator")) {
      ImGui::MenuItem("Auto-Start", nullptr, &auto_start);
      ImGui::Separator();
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
      ImGui::Separator();
      if (ImGui::MenuItem("Reset")) {
        emulator.reset();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    rlImGuiEnd();

    render_error();

    EndDrawing();
  }

  spdlog::info("Shutting down...");
}

void Interface::render_error() {
  if (error_message.empty()) {
    return;
  }

  auto text_width = MeasureText(error_message.c_str(), 15);
  auto text_height = 20;
  auto text_x = (kDefaulWindowWidth - text_width) / 2;
  auto text_y = (kDefaultWindowHeight / 2) - (text_height / 2);
  auto padding = 10;

  DrawRectangle(text_x - padding, text_y - padding, text_width + padding + padding, text_height + padding + padding, Color{32,32,32,127});
  DrawText(error_message.c_str(), text_x, text_y, 15, RED);
}

void Interface::load_cartridge() {
  nfdchar_t* file_path = nullptr;
  std::array<nfdfilteritem_t, 1> filter_items = {{
    {"GB", "gb"},
  }};

  nfdresult_t result = NFD_OpenDialog(&file_path, filter_items.data(), filter_items.size(), nullptr);
  if (result == NFD_OKAY) {
    fs::path path{file_path};

    auto load_result = load_bin(path.string());
    if (!load_result) {
      error_message = load_result.error();
      spdlog::error("{}", error_message);
      return;
    }

    emulator.load_cartridge(std::move(load_result.value()));
    spdlog::info("Loaded cartridge: '{}'", fs::absolute(path).string());

    if (auto_start) {
      emulator.play();
    }
  } else if (result == NFD_CANCEL) {
    spdlog::info("Load cancelled by user.");
  } else {
    spdlog::error("Loading failed: {}", NFD_GetError());
  }
}

void Interface::render_info() {
  DrawText(fmt::format("{}", emulator.registers()).c_str(), 20, 32, 12, RAYWHITE);
  DrawText(fmt::format("cycles={}", emulator.cycles()).c_str(), 20, 48, 12, RAYWHITE);
  DrawText(fmt::format("mode={}", magic_enum::enum_name(emulator.mode())).c_str(), 20, 64, 12, RAYWHITE);
//    DrawText(fmt::format("stat={}", emulator.read8(std::to_underlying(IO::STAT))).c_str(), 20, 80, 12, RAYWHITE);

  Instruction instr = emulator.instr();
  DrawText(fmt::format("opcode={}, bytes={}, cycles={}/{}", magic_enum::enum_name(instr.opcode), instr.bytes, instr.cycles, instr.cycles_cond).c_str() , 20, 96, 12, RAYWHITE);

  uint8_t pc = emulator.registers().pc;
  DrawText(fmt::format("PC=0x{:02x}", pc).c_str() , 20, 128, 12, RAYWHITE);
  DrawText(fmt::format("@PC+0={:02x}", emulator.read8(pc)).c_str() , 20, 144, 12, RAYWHITE);
  DrawText(fmt::format("@PC+1={:02x}", emulator.read8(pc+1)).c_str() , 20, 160, 12, RAYWHITE);
  DrawText(fmt::format("@PC+2={:02x}", emulator.read8(pc+2)).c_str() , 20, 176, 12, RAYWHITE);
  DrawText(fmt::format("@PC+4={:02x}", emulator.read8(pc+3)).c_str() , 20, 192, 12, RAYWHITE);
}
