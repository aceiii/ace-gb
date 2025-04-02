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
constexpr char const *kWindowTitle = "Ace::GB - GameBoy Emulator";

constexpr int kAudioSampleRate = 48000;
constexpr int kAudioSampleSize = 32;
constexpr int kAudioNumChannels = 2;
constexpr int kSamplesPerUpdate = 4096;

AudioStream stream;
}

void rlImGuiImageTextureFit(const Texture2D *image, bool center) {
  if (!image)
    return;

  ImVec2 area = ImGui::GetContentRegionAvail();

  float scale = area.x / image->width;

  float y = image->height * scale;
  if (y > area.y) {
    scale = area.y / image->height;
  }

  int sizeX = image->width * scale;
  int sizeY = image->height * scale;

  if (center) {
    ImGui::SetCursorPosX(0);
    ImGui::SetCursorPosX(area.x / 2 - sizeX / 2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (area.y / 2 - sizeY / 2));
  }

  rlImGuiImageRect(image, sizeX, sizeY, Rectangle { 0, 0, static_cast<float>(image->width), static_cast<float>(image->height) });
}

Interface::Interface(): emulator {{ kAudioSampleRate, kSamplesPerUpdate, kAudioNumChannels }} {
  spdlog::info("Initializing interface");

  NFD_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(kDefaulWindowWidth, kDefaultWindowHeight, kWindowTitle);
  InitAudioDevice();

  SetAudioStreamBufferSizeDefault(kSamplesPerUpdate);
  stream = LoadAudioStream(kAudioSampleRate, kAudioSampleSize, kAudioNumChannels);

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
  reset();

  spdlog::info("Running...");

  static bool should_close = false;
  static bool show_lcd = true;
  static bool show_tiles = true;
  static bool show_tilemap1 = true;
  static bool show_tilemap2 = true;
  static bool show_sprites = true;
  static bool show_cpu_registers = true;

  static bool enable_audio = true;

  auto &io = ImGui::GetIO();

  while (!should_close) {
    const auto frame_time = GetFrameTime();

    if (WindowShouldClose()) {
      should_close = true;
    }

    if (IsFileDropped()) {
      FilePathList dropped_files = LoadDroppedFiles();

      if (dropped_files.count > 1) {
        spdlog::warn("Loading only last dropped file");
      }

      auto idx = dropped_files.count - 1;
      std::string file_path { dropped_files.paths[idx] };
      load_cart_rom(file_path);

      UnloadDroppedFiles(dropped_files);
    }

    emulator.update_input(JoypadButton::Up, IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
    emulator.update_input(JoypadButton::Down, IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S));
    emulator.update_input(JoypadButton::Left, IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
    emulator.update_input(JoypadButton::Right, IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D));
    emulator.update_input(JoypadButton::Select, IsKeyDown(KEY_U) || IsKeyDown(KEY_RIGHT_SHIFT));
    emulator.update_input(JoypadButton::Start, IsKeyDown(KEY_I) || IsKeyDown(KEY_ENTER));
    emulator.update_input(JoypadButton::A, IsKeyDown(KEY_J) || IsKeyDown(KEY_Z));
    emulator.update_input(JoypadButton::B, IsKeyDown(KEY_K) || IsKeyDown(KEY_X));

    emulator.update(frame_time);

    BeginDrawing();
    ClearBackground(DARKGRAY);

    rlImGuiBegin();

    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (show_lcd) {
      render_lcd(show_lcd);
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
      if (ImGui::MenuItem("Skip Boot ROM", nullptr, emulator.skip_bootrom())) {
        emulator.set_skip_bootrom(!emulator.skip_bootrom());
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Play", nullptr, nullptr, !emulator.is_playing())) {
        play();
      }
      if (ImGui::MenuItem("Stop", nullptr, nullptr, emulator.is_playing())) {
        stop();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Step", nullptr, nullptr, !emulator.is_playing())) {
        step();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Reset")) {
        reset();
        if (auto_start) {
          play();
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Hardware")) {
      if (ImGui::BeginMenu("PPU")) {
        ImGui::MenuItem("LCD", nullptr, &show_lcd);
        ImGui::MenuItem("TileMap 1", nullptr, &show_tilemap1);
        ImGui::MenuItem("TileMap 2", nullptr, &show_tilemap2);
        ImGui::MenuItem("Sprites", nullptr, &show_sprites);
        ImGui::MenuItem("Tiles", nullptr, &show_tiles);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("APU")) {
        if (ImGui::MenuItem("Enable Sound", nullptr, emulator.channel_enabled(AudioChannelID::MASTER))) {
          emulator.toggle_channel(AudioChannelID::MASTER, !emulator.channel_enabled(AudioChannelID::MASTER));
        }
        if (ImGui::BeginMenu("Volume")) {
          auto volume = GetMasterVolume() * 100;
          ImGui::Text("%d%%", static_cast<int>(volume));
          if (ImGui::MenuItem("Up")) {
            volume = std::clamp(volume + 10, 0.f, 100.f);
            SetMasterVolume(volume / 100.f);
          }
          if (ImGui::MenuItem("Down")) {
            volume = std::clamp(volume - 10, 0.0f, 100.0f);
            SetMasterVolume(volume / 100.f);
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Channels")) {
          if (ImGui::MenuItem("CH1 - Square", nullptr, emulator.channel_enabled(AudioChannelID::CH1))) {
            emulator.toggle_channel(AudioChannelID::CH1, !emulator.channel_enabled(AudioChannelID::CH1));
          }
          if (ImGui::MenuItem("CH2 - Square", nullptr, emulator.channel_enabled(AudioChannelID::CH2))) {
            emulator.toggle_channel(AudioChannelID::CH2, !emulator.channel_enabled(AudioChannelID::CH2));
          }
          if (ImGui::MenuItem("CH3 - Wave", nullptr, emulator.channel_enabled(AudioChannelID::CH3))) {
            emulator.toggle_channel(AudioChannelID::CH3, !emulator.channel_enabled(AudioChannelID::CH3));
          }
          if (ImGui::MenuItem("CH4 - Noise", nullptr, emulator.channel_enabled(AudioChannelID::CH4))) {
            emulator.toggle_channel(AudioChannelID::CH4, !emulator.channel_enabled(AudioChannelID::CH4));
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    rlImGuiEnd();

    render_error();

    DrawFPS(10, GetScreenHeight() - 24);

    EndDrawing();

    if (IsAudioStreamPlaying(stream) && IsAudioStreamProcessed(stream)) {
      std::array<float, kAudioNumChannels * kSamplesPerUpdate> samples {};
      emulator.audio_samples(samples.data(), samples.size() / kAudioNumChannels, kAudioNumChannels);
      UpdateAudioStream(stream, samples.data(), kSamplesPerUpdate);
    }
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

  DrawRectangle(text_x - padding, text_y - padding, text_width + padding + padding, text_height + padding + padding, Color { 32, 32, 32, 127 });
  DrawText(error_message.c_str(), text_x, text_y, 15, RED);
}

void Interface::load_cartridge() {
  nfdchar_t *file_path = nullptr;
  std::array<nfdfilteritem_t, 3> filter_items = {{
    { "GB", "gb" },
    { "BIN", "bin" },
    { "ROM", "rom" }
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
  fs::path path { file_path };
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
    play();
  }
}

void Interface::render_lcd(bool &show_window) {
  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  ImGui::Begin("GameBoy", &show_window);
  {
    rlImGuiImageTextureFit(&emulator.target_lcd(), true);
  }
  ImGui::End();
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
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
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
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
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
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
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
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
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
    ImGui::Text("AF=%04X BC=%04X DE=%04X HL=%04X", af, bc, de, hl);
    ImGui::Text("Flags Z=%d N=%d H=%d C=%d", zero, neg, half_carry, carry);

    const auto &state = emulator.state();
    ImGui::Text("State IME=%d HALT=%d STOP=%d HARD_LOCK=%d", state.ime, state.halt, state.stop, state.hard_lock);
  }
  ImGui::End();
}

void Interface::play() {
  emulator.play();
  PlayAudioStream(stream);
}

void Interface::stop() {
  emulator.stop();
  StopAudioStream(stream);
}

void Interface::step() {
  emulator.step();
}

void Interface::reset() {
  emulator.reset();
  StopAudioStream(stream);
}
