#include <filesystem>
#include <string>
#include <raylib.h>
#include <imgui.h>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>

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

  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

  rlImGuiShutdown();

  CloseAudioDevice();
  CloseWindow();
}

void Interface::run() {
  emulator.reset();
  emulator.add_breakpoint(0x150);

  spdlog::info("Running...");

  static bool should_close = false;
  static bool show_lcd = true;
  static bool show_tiles = true;
  static bool show_tilemap1 = true;
  static bool show_tilemap2 = true;
  static bool show_sprites = true;
  static bool show_cpu_registers = true;

  auto &io = ImGui::GetIO();

  while (!should_close) {
    if (WindowShouldClose()) {
      should_close = true;
    }

    if (IsFileDropped()) {
      FilePathList dropped_files = LoadDroppedFiles();

      if (dropped_files.count > 1) {
        spdlog::warn("Loading only last dropped file");
      }

      auto idx = dropped_files.count - 1;
      std::string file_path {dropped_files.paths[idx]};
      load_cart_rom(file_path);

      UnloadDroppedFiles(dropped_files);
    }

    emulator.update();

    BeginDrawing();
    ClearBackground(DARKGRAY);

    render_info();

    rlImGuiBegin();

    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (show_lcd) {
      emulator.render(show_lcd);
    }

    if (show_tiles) {
      render_tiles(show_tiles);
    }

    if (show_tilemap1) {
      render_tilemap1(show_tilemap1);
    }

    if (show_tilemap2) {
      render_tilemap2(show_tilemap2);
    }

    if (show_sprites) {
      render_sprites(show_sprites);
    }

    if (show_cpu_registers) {
      render_registers(show_cpu_registers);
    }

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
    if (ImGui::BeginMenu("Hardware")) {
      if (ImGui::BeginMenu("PPU")) {
        ImGui::MenuItem("Stat", nullptr, &show_lcd);
        ImGui::MenuItem("VRAM");
        ImGui::MenuItem("OAM");
        ImGui::MenuItem("TileMap 1", nullptr, &show_tilemap1);
        ImGui::MenuItem("TileMap 2", nullptr, &show_tilemap2);
        ImGui::MenuItem("Sprites");
        ImGui::MenuItem("Tiles", nullptr, &show_tiles);
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    rlImGuiEnd();

    render_error();

    DrawFPS(10, GetScreenHeight() - 24);

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
  std::array<nfdfilteritem_t, 3> filter_items = {{
    {"GB", "gb"},
    {"BIN", "bin"},
    {"ROM", "rom"}
  }};

  nfdresult_t result = NFD_OpenDialog(&file_path, filter_items.data(), filter_items.size(), nullptr);
  if (result == NFD_OKAY) {
    load_cart_rom(file_path);
  } else if (result == NFD_CANCEL) {
    spdlog::info("Load cancelled by user.");
  } else {
    spdlog::error("Loading failed: {}", NFD_GetError());
  }
}

void Interface::load_cart_rom(const std::string &file_path) {
  fs::path path{file_path};
  auto ext = path.extension();
  if (ext != ".gb" && ext != ".bin" && ext != ".rom") {
    spdlog::error("Invalid file extension: {}. Only supports loading .gb, .bin, .rom", ext.string());
    return;
  }

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
}

void Interface::render_info() {
//  DrawText(fmt::format("{}", emulator.registers()).c_str(), 20, 32, 12, RAYWHITE);
//  DrawText(fmt::format("cycles={}", emulator.cycles()).c_str(), 20, 48, 12, RAYWHITE);
//  DrawText(fmt::format("mode={}", magic_enum::enum_name(emulator.mode())).c_str(), 20, 64, 12, RAYWHITE);
//    DrawText(fmt::format("stat={}", emulator.read8(std::to_underlying(IO::STAT))).c_str(), 20, 80, 12, RAYWHITE);

//  Instruction instr = emulator.instr();
//  DrawText(fmt::format("opcode={}, bytes={}, cycles={}/{}", magic_enum::enum_name(instr.opcode), instr.bytes, instr.cycles, instr.cycles_cond).c_str() , 20, 96, 12, RAYWHITE);

//  uint8_t pc = emulator.registers().pc;
//  DrawText(fmt::format("PC=0x{:02x}", pc).c_str() , 20, 128, 12, RAYWHITE);
//  DrawText(fmt::format("@PC+0={:02x}", emulator.read8(pc)).c_str() , 20, 144, 12, RAYWHITE);
//  DrawText(fmt::format("@PC+1={:02x}", emulator.read8(pc+1)).c_str() , 20, 160, 12, RAYWHITE);
//  DrawText(fmt::format("@PC+2={:02x}", emulator.read8(pc+2)).c_str() , 20, 176, 12, RAYWHITE);
//  DrawText(fmt::format("@PC+4={:02x}", emulator.read8(pc+3)).c_str() , 20, 192, 12, RAYWHITE);

//  const auto &state = emulator.state();
//  DrawText(fmt::format("CPU State ime={}, halt={}, stop={}, hard_lock={}", state.ime, state.halt, state.stop, state.hard_lock).c_str(), 20, 224, 12, RAYWHITE);
}

void Interface::render_tiles(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Tile Data", &show_window)) {
    auto &target = emulator.target_tiles();
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 3;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle{ 0,0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::render_tilemap1(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("TileMap 1", &show_window)) {
    auto &target = emulator.target_tilemap(0);
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle{ 0,0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::render_tilemap2(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("TileMap 2", &show_window)) {
    auto &target = emulator.target_tilemap(1);
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle{ 0,0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::render_sprites(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Sprites", &show_window)) {
    auto &target = emulator.target_sprites();
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle{ 0,0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::render_registers(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Registers", &show_window)) {
    const auto &regs = emulator.registers();

    auto a = regs.get(Reg8::A);
    auto f = regs.get(Reg8::F);
    auto b = regs.get(Reg8::B);
    auto c = regs.get(Reg8::C);
    auto d = regs.get(Reg8::D);
    auto e = regs.get(Reg8::E);
    auto h = regs.get(Reg8::H);
    auto l = regs.get(Reg8::L);
    auto af = regs.get(Reg16::AF);
    auto bc = regs.get(Reg16::BC);
    auto de = regs.get(Reg16::DE);
    auto hl = regs.get(Reg16::HL);
    auto zero = regs.get(Flag::Z);
    auto neg = regs.get(Flag::N);
    auto half_carry = regs.get(Flag::H);
    auto carry = regs.get(Flag::C);

    ImGui::Text("PC=%04X (%d)", regs.pc, regs.pc);
    ImGui::Text("SP=%04X (%d)", regs.sp, regs.sp);
    ImGui::Text("A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X", a, f, b, c, d, e, h, l);
    ImGui::Text("AF=%04X BC=%04X DE=%04X HL=%04X", af, bc, de,hl);
    ImGui::Text("Flags Z=%d N=%d H=%d C=%d", zero, neg, half_carry, carry);
  }
  ImGui::End();
}
