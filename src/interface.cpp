#include <filesystem>
#include <string>
#include <raylib.h>
#include <imgui.h>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <toml++/toml.hpp>

#include "interface.h"
#include "emulator.h"
#include "file.h"


namespace fs = std::filesystem;

using namespace app;

namespace {
constexpr int kDefaultWindowWidth = 800;
constexpr int kDefaultWindowHeight = 600;
constexpr char const *kWindowTitle = "Ace::GB - GameBoy Emulator";

constexpr int kAudioSampleRate = 48000;
constexpr int kAudioSampleSize = 32;
constexpr int kAudioNumChannels = 2;
constexpr int kSamplesPerUpdate = 512;

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

static auto SerializeInterfaceSettings(const InterfaceSettings& settings) -> toml::table {
  toml::array recent_files {};
  for (const auto &filename : settings.recent_files) {
    recent_files.push_back(filename);
  }

  auto table = toml::table{
    { "window", toml::table{
        { "x", settings.screen_x },
        { "y", settings.screen_y },
        { "width", settings.screen_width },
        { "height", settings.screen_height },
      }
    },
    { "file", toml::table{
        { "recent_files", recent_files },
      }
    }
  };

  return table;
}

static auto DeserializeInterfaceSettings(const toml::table& table, InterfaceSettings& settings) -> void {
  settings.screen_x = table["window"]["x"].value_or(-1);
  settings.screen_y = table["window"]["y"].value_or(-1);

  settings.screen_width = table["window"]["width"].value_or(kDefaultWindowWidth);
  if (settings.screen_width < 100) {
    settings.screen_width = kDefaultWindowWidth;
  }

  settings.screen_height = table["window"]["height"].value_or(kDefaultWindowHeight);
  if (settings.screen_height < 100) {
    settings.screen_height = kDefaultWindowHeight;
  }

  if (auto arr = table["file"]["recent_files"].as_array()) {
    settings.recent_files.Clear();
    arr->for_each([&](auto&& file) {
      if constexpr (toml::is_string<decltype(file)>) {
        settings.recent_files.Push(*file);
      }
    });
  }
}

static auto SpdLogTraceLog(int log_level, const char* text, va_list args) -> void {
  static std::array<char, 2048> buffer;
  vsnprintf(buffer.data(), buffer.size(), text, args);
  switch (log_level) {
    case LOG_TRACE: spdlog::trace("[RAYLIB] {}", buffer.data()); break;
    case LOG_DEBUG: spdlog::debug("[RAYLIB] {}", buffer.data()); break;
    case LOG_INFO: spdlog::info("[RAYLIB] {}", buffer.data()); break;
    case LOG_WARNING: spdlog::warn("[RAYLIB] {}", buffer.data()); break;
    case LOG_ERROR: spdlog::error("[RAYLIB] {}", buffer.data()); break;
    case LOG_FATAL: spdlog::critical("[RAYLIB] {}", buffer.data()); break;
    default: spdlog::info("[RAYLIB] {}", buffer.data()); break;
  }
}

Interface::Interface(Args args)
    : args{args},
      emulator{{ .sample_rate=kAudioSampleRate, .buffer_size=kSamplesPerUpdate, .num_channels=kAudioNumChannels }},
      config{.serialize=SerializeInterfaceSettings, .deserialize=DeserializeInterfaceSettings}
{
  SetTraceLogCallback(SpdLogTraceLog);

  spdlog::info("Initializing interface");

  if (auto res = config.Load(args.settings_filename); !res.has_value()) {
    spdlog::warn("Failed to load settings file '{}': {}", args.settings_filename, res.error());
  }

  NFD_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(config.settings.screen_width, config.settings.screen_height, kWindowTitle);
  if (config.settings.screen_x >= 0 && config.settings.screen_y >= 0) {
    SetWindowPosition(config.settings.screen_x, config.settings.screen_y);
  }

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
  static bool show_input = true;

  static bool enable_audio = true;

  auto &io = ImGui::GetIO();

  while (!should_close) {
    if (WindowShouldClose()) {
      should_close = true;
    }

    const auto frame_time = GetFrameTime();
    config.settings.screen_width = GetScreenWidth();
    config.settings.screen_height = GetScreenHeight();

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

    if (show_input) {
      render_input(show_input);
    }

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load Cartridge")) {
        spdlog::info("Loading cart...");
        load_cartridge();
      }

      auto& recent_files = config.settings.recent_files;
      if (ImGui::BeginMenu("Recent files", !recent_files.IsEmpty())) {
        for (auto &file : recent_files) {
          if (ImGui::MenuItem(fs::relative(file).c_str())) {
            spdlog::debug("Pressed recent file: '{}'", file);
            load_cart_rom(file);
          }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear Recent")) {
          spdlog::debug("Clearing recent files");
          recent_files.Clear();
        }
        ImGui::EndMenu();
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

    DrawFPS(10, config.settings.screen_height - 24);

    EndDrawing();

    if (IsAudioStreamPlaying(stream) && IsAudioStreamProcessed(stream)) {
      // static auto prev = std::chrono::high_resolution_clock::now();
      // auto current = std::chrono::high_resolution_clock::now();
      // auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(current - prev);
      // prev = current;

      auto &samples = emulator.audio_samples();
      UpdateAudioStream(stream, samples.data(), samples.size() / kAudioNumChannels);
    }
  }

  cleanup();

  spdlog::info("Shutting down...");
}

void Interface::render_error() {
  if (error_message.empty()) {
    return;
  }

  auto text_width = MeasureText(error_message.c_str(), 15);
  auto text_height = 20;
  auto text_x = (config.settings.screen_width - text_width) / 2;
  auto text_y = (config.settings.screen_height / 2) - (text_height / 2);
  auto padding = 10;

  DrawRectangle(text_x - padding, text_y - padding, text_width + padding + padding, text_height + padding + padding, Color { 32, 32, 32, 127 });
  DrawText(error_message.c_str(), text_x, text_y, 15, RED);
}

void Interface::load_cartridge() {
  nfdchar_t *file_path = nullptr;
  std::array<nfdfilteritem_t, 3> filter_items = {{
    { .name="GB", .spec="gb" },
    { .name="BIN", .spec="bin" },
    { .name="ROM", .spec="rom" }
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
  stop();

  fs::path path { file_path };
  auto ext = path.extension();
  if (ext != ".gb" && ext != ".bin" && ext != ".rom") {
    config.settings.recent_files.Remove(path);
    spdlog::error("Invalid file extension: {}. Only supports loading .gb, .bin, .rom", ext.string());
    return;
  }

  auto load_result = load_bin(path.string());
  if (!load_result) {
    config.settings.recent_files.Remove(path);
    error_message = load_result.error();
    spdlog::error("{}", error_message);
    return;
  }

  emulator.load_cartridge(std::move(load_result.value()));
  spdlog::info("Loaded cartridge: '{}'", fs::absolute(path).string());

  if (auto_start) {
    play();
  }

  config.settings.recent_files.Push(path.string());
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

void Interface::render_input(bool &show_window) {
  if (!show_window) {
    return;
  }

  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Input", &show_window)) {

    static constexpr auto alignForWidth = [](float width, float alignment = 0.5f) {
      ImGuiStyle& style = ImGui::GetStyle();
      float avail = ImGui::GetContentRegionAvail().x;
      float off = (avail - width) * alignment;
      if (off > 0.0f) {
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
      }
    };

    static constexpr auto drawMaybeDisabled = [](bool disabled, auto draw) {
      if (disabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
      }
      draw();
      if (disabled) {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
      }
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, -4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    alignForWidth(288);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Up), []() {
      ImGui::Button(ICON_FA_CARET_UP, { 32, 32 });
    });

    ImGui::SameLine(0, 32);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Select), []() {
      ImGui::Button("select");
    });
    ImGui::SameLine(0, 4);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Start), []() {
      ImGui::Button("start");
    });
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::NewLine();
    alignForWidth(288);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Left), []() {
      ImGui::Button(ICON_FA_CARET_LEFT, { 32, 32 });
    });
    ImGui::SameLine(0, 32);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Right), []() {
      ImGui::Button(ICON_FA_CARET_RIGHT, { 32, 32 });
    });

    ImGui::SameLine(0, 64);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 24.0f);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::B), []() {
      ImGui::Button("B", { 32, 32 });
    });
    ImGui::SameLine();
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::A), []() {
      ImGui::Button("A", { 32, 32 });
    });
    ImGui::PopStyleVar();

    ImGui::NewLine();
    alignForWidth(288);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
    drawMaybeDisabled(emulator.is_pressed(JoypadButton::Down), []() {
      ImGui::Button(ICON_FA_CARET_DOWN, { 32, 32 });
    });

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
  }
  ImGui::End();
}

void Interface::cleanup() {
  auto window_pos = GetWindowPosition();
  config.settings.screen_x = static_cast<int>(window_pos.x);
  config.settings.screen_y = static_cast<int>(window_pos.y);

  if (auto res = config.Save(args.settings_filename); !res.has_value()) {
    spdlog::warn("Failed to save settings to file '{}': {}", args.settings_filename, res.error());
  }
}

void Interface::play() {
  emulator.play();
  SetAudioStreamBufferSizeDefault(kSamplesPerUpdate);
  stream = LoadAudioStream(kAudioSampleRate, kAudioSampleSize, kAudioNumChannels);
  PlayAudioStream(stream);
}

void Interface::stop() {
  emulator.stop();
  StopAudioStream(stream);
  UnloadAudioStream(stream);
}

void Interface::step() {
  emulator.step();
}

void Interface::reset() {
  emulator.reset();
  StopAudioStream(stream);
}
