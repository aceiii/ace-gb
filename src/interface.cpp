#include <filesystem>
#include <span>
#include <string>
#include <raylib.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <rlImGui.h>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <toml++/toml.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "interface.hpp"
#include "emulator.hpp"
#include "file.hpp"


namespace fs = std::filesystem;

using namespace app;

namespace {
constexpr int kDefaultWindowWidth = 800;
constexpr int kDefaultWindowHeight = 600;
constexpr char const* kWindowTitle = "Ace::GB - GameBoy Emulator";
constexpr char const* kDefaultBootRomPath = "boot.bin";

constexpr int kAudioSampleRate = 44100;
constexpr int kAudioSampleSize = 32;
constexpr int kAudioNumChannels = 2;
constexpr int kSamplesPerUpdate = 512;

constexpr char const* kBootRomErrorKey = "BootRomError";
constexpr char const* kCartRomErrorKey = "CartRomError";

constexpr int kTargetEmulatorFrameRate = 60;
constexpr double kTargetEmulatorFrameTime = 1.0 / kTargetEmulatorFrameRate;

constexpr int kLockedFrameRate = 60;

AudioStream stream;

std::function<void(std::span<float>)> g_audio_callback;
}

void rlImGuiImageTextureFit(const Texture2D* image, bool center) {
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
  for (const auto& filename : settings.recent_files) {
    recent_files.push_back(filename);
  }

  auto table = toml::table{
    {
      "window",
      toml::table{
        { "x", settings.screen_x },
        { "y", settings.screen_y },
        { "width", settings.screen_width },
        { "height", settings.screen_height },
        { "reset_view", settings.reset_view },
        { "lock_framerate", settings.lock_framerate },
      },
    },
    {
      "file",
      toml::table{
        { "recent_files", recent_files },
      },
    },
    {
      "emulator",
      toml::table{
        { "auto_start", settings.auto_start },
        { "skip_boot_rom", settings.skip_boot_rom },
        { "boot_rom_path", settings.boot_rom_path },
      },
    },
    {
      "hardware",
      toml::table{
        { "show_lcd", settings.show_lcd },
        { "show_tiles", settings.show_tiles },
        { "show_tilemap1", settings.show_tilemap1 },
        { "show_tilemap2", settings.show_tilemap2 },
        { "show_sprites", settings.show_sprites },
        { "show_cpu_registers", settings.show_cpu_registers },
        { "show_input", settings.show_input },
        { "show_memory", settings.show_memory },
        { "show_instructions", settings.show_instructions },
        { "show_logs", settings.show_logs },
        { "enable_audio", settings.enable_audio },
        { "enable_ch1", settings.enable_ch1 },
        { "enable_ch2", settings.enable_ch2 },
        { "enable_ch3", settings.enable_ch3 },
        { "enable_ch4", settings.enable_ch4 },
        { "master_volume", settings.master_volume },
      }
    },
  };

  return table;
}

static auto DeserializeInterfaceSettings(const toml::table& table, InterfaceSettings& settings) -> void {
  settings.lock_framerate = table["window"]["lock_framerate"].value_or(false);
  settings.reset_view = table["window"]["reset_view"].value_or(true);
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

  settings.auto_start = table["emulator"]["auto_start"].value_or(true);
  settings.skip_boot_rom = table["emulator"]["skip_boot_rom"].value_or(true);
  settings.boot_rom_path = table["emulator"]["boot_rom_path"].value_or(std::string{kDefaultBootRomPath});

  settings.show_lcd = table["hardware"]["show_lcd"].value_or(true);
  settings.show_tiles = table["hardware"]["show_tiles"].value_or(true);
  settings.show_tilemap1 = table["hardware"]["show_tilemap1"].value_or(true);
  settings.show_tilemap2 = table["hardware"]["show_tilemap2"].value_or(true);
  settings.show_sprites = table["hardware"]["show_sprites"].value_or(true);
  settings.show_cpu_registers = table["hardware"]["show_cpu_registers"].value_or(true);
  settings.show_input = table["hardware"]["show_input"].value_or(true);
  settings.show_memory = table["hardware"]["show_memory"].value_or(true);
  settings.show_instructions = table["hardware"]["show_instructions"].value_or(true);
  settings.show_logs = table["hardware"]["show_logs"].value_or(true);

  settings.enable_audio = table["hardware"]["enable_audio"].value_or(true);
  settings.enable_ch1 = table["hardware"]["enable_ch1"].value_or(true);
  settings.enable_ch2 = table["hardware"]["enable_ch2"].value_or(true);
  settings.enable_ch3 = table["hardware"]["enable_ch3"].value_or(true);
  settings.enable_ch4 = table["hardware"]["enable_ch4"].value_or(true);
  settings.master_volume = std::clamp(table["hardware"]["master_volume"].value_or(100.0f), 0.0f, 100.0f);
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

static uint8_t MemEditorCustomRead(const uint8_t* data, size_t offset, void* user_data) {
  auto emulator = static_cast<Emulator*>(user_data);
  return emulator->Read8(offset);
}

static void MemEditorCustomWrite(uint8_t* data, size_t offset, uint8_t d, void* user_data) {
  auto emulator = static_cast<Emulator*>(user_data);
  emulator->Write8(offset, d);
}

static uint32_t MemEditorCustomBgColor(const uint8_t* data, size_t offset, void* user_data) {
  auto emulator = static_cast<Emulator*>(user_data);

  uint16_t pc = emulator->GetRegisters().pc;
  if (pc == offset || pc == offset + 1) {
    return IM_COL32(128, 24, 21, 255);
  }

  uint16_t sp = emulator->GetRegisters().sp;
  if (sp == offset || sp == offset + 1) {
    return IM_COL32(32, 72, 128, 255);
  }

  return IM_COL32(0, 0, 0, 0);
}

static void AudioInputCallback(void *buffer, unsigned int frames) {
  std::span<float> buffer_span{static_cast<float*>(buffer), frames * kAudioNumChannels};
  g_audio_callback(buffer_span);
}

Interface::Interface(Args args)
    : args_{args},
      emulator_{{ .sample_rate=kAudioSampleRate, .buffer_size=kSamplesPerUpdate, .num_channels=kAudioNumChannels }},
      config_{.serialize=SerializeInterfaceSettings, .deserialize=DeserializeInterfaceSettings},
      error_messages_{}
{
  mem_editor_.ReadFn = MemEditorCustomRead;
  mem_editor_.WriteFn = MemEditorCustomWrite;
  mem_editor_.BgColorFn = MemEditorCustomBgColor;
  mem_editor_.UserData = static_cast<void*>(&emulator_);

  assembly_viewer_.Initialize(&emulator_);

  SetTraceLogCallback(SpdLogTraceLog);

  auto level = spdlog::get_level();
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto formatter = std::make_shared<spdlog::pattern_formatter>();
  auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
      [=, this](const spdlog::details::log_msg& msg) {
        spdlog::memory_buf_t formatted;
        formatter->format(msg, formatted);
        app_log_.AddLog(formatted);
      });

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(console_sink);
  sinks.push_back(callback_sink);

  auto logger =
      std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
  logger->set_level(level);

  spdlog::set_default_logger(logger);

  spdlog::info("Initializing interface");

  if (auto res = config_.Load(args.settings_filename); !res.has_value()) {
    spdlog::warn("Failed to load settings file '{}': {}", args.settings_filename, res.error());
  }

  NFD_Init();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(config_.settings.screen_width, config_.settings.screen_height, kWindowTitle);
  if (config_.settings.screen_x >= 0 && config_.settings.screen_y >= 0) {
    SetWindowPosition(config_.settings.screen_x, config_.settings.screen_y);
  }

  InitAudioDevice();
  SetMasterVolume(config_.settings.master_volume / 100.0f);

  SetAudioStreamBufferSizeDefault(kSamplesPerUpdate);
  stream = LoadAudioStream(kAudioSampleRate, kAudioSampleSize, kAudioNumChannels);
  SetAudioStreamCallback(stream, AudioInputCallback);
  g_audio_callback = std::bind(&Emulator::OnAudioCallback, &emulator_, std::placeholders::_1);

  int monitor = GetCurrentMonitor();
  spdlog::trace("Current monitor: {}", monitor);

  int monitor_width = GetMonitorWidth(monitor);
  int monitor_height = GetMonitorHeight(monitor);
  spdlog::trace("Monitor resolution: {}x{}", monitor_width, monitor_height);

  SetExitKey(KEY_NULL);
  rlImGuiSetup(true);

  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  emulator_.Init();

  if (auto result = emulator_.SetBootRomPath(config_.settings.boot_rom_path); !result) {
    spdlog::error("Failed to set boot rom path");
    error_messages_.AddError(kBootRomErrorKey, result.error());
  } else {
    error_messages_.ClearError(kBootRomErrorKey);
  }

  while (!IsWindowReady()) {
    // pass
  }

  if (config_.settings.lock_framerate) {
    SetTargetFPS(kLockedFrameRate);
  }
}

Interface::~Interface() {
  spdlog::info("Cleaning up interface");

  emulator_.Cleanup();

  rlImGuiShutdown();

  CloseAudioDevice();
  CloseWindow();
}

void Interface::Run() {
  Reset();

  spdlog::info("Running...");

  emulator_.SetSkipBootRom(config_.settings.skip_boot_rom);

  while (!should_close_) {
    Update();
  }

  Cleanup();

  spdlog::info("Shutting down...");
}

void Interface::Update() {
  ZoneScoped;

  if (WindowShouldClose()) {
    should_close_ = true;
  }

  const auto frame_time = GetFrameTime();
  config_.settings.screen_width = GetScreenWidth();
  config_.settings.screen_height = GetScreenHeight();

  if (IsFileDropped()) {
    FilePathList dropped_files = LoadDroppedFiles();

    if (dropped_files.count > 1) {
      spdlog::warn("Loading only last dropped file");
    }

    auto idx = dropped_files.count - 1;
    std::string file_path { dropped_files.paths[idx] };
    LoadCartRom(file_path);

    UnloadDroppedFiles(dropped_files);
  }

  emulator_.UpdateInput(JoypadButton::Up, IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
  emulator_.UpdateInput(JoypadButton::Down, IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S));
  emulator_.UpdateInput(JoypadButton::Left, IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
  emulator_.UpdateInput(JoypadButton::Right, IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D));
  emulator_.UpdateInput(JoypadButton::Select, IsKeyDown(KEY_U) || IsKeyDown(KEY_RIGHT_SHIFT));
  emulator_.UpdateInput(JoypadButton::Start, IsKeyDown(KEY_I) || IsKeyDown(KEY_ENTER));
  emulator_.UpdateInput(JoypadButton::B, IsKeyDown(KEY_J) || IsKeyDown(KEY_Z));
  emulator_.UpdateInput(JoypadButton::A, IsKeyDown(KEY_K) || IsKeyDown(KEY_X));

  static double last_update = 0;
  last_update += GetFrameTime();
  while (last_update >= kTargetEmulatorFrameTime) {
    emulator_.Update(frame_time);
    last_update -= kTargetEmulatorFrameTime;
  }

  BeginDrawing();
  ClearBackground(DARKGRAY);
  {
    ZoneScopedN("ImGuiBegin");
    rlImGuiBegin();
    ConfigureDockSpace();
    RenderLCD();
    RenderTiles();
    RenderTilemap1();
    RenderTilemap2();
    RenderSprites();
    RenderRegisters();
    RenderInput();
    RenderMemory();
    RenderInstructions();
    RenderLogs();
    RenderMainMenu();
    RenderStatusBar();
    RenderSettingsPopup();
    {
      ZoneScopedN("ImGuiEnd");
      rlImGuiEnd();
    }
  }
  RenderError();
  EndDrawing();

  FrameMark;
}

void Interface::ConfigureDockSpace() {
  ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  if (config_.settings.reset_view) {
    config_.settings.reset_view = false;

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID main = dockspace_id;
    ImGuiID right = ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.28f, nullptr, &main);
    ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.4f, nullptr, &right);
    ImGuiID right_center = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.32f, nullptr, &right);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(main, ImGuiDir_Down, 0.32f, nullptr, &main);
    ImGuiID center_top = ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.33f, nullptr, &main);
    ImGuiID center_bottom = ImGui::DockBuilderSplitNode(center_top, ImGuiDir_Down, 0.45f, nullptr, &center_top);
    ImGuiID left = ImGui::DockBuilderSplitNode(main, ImGuiDir_Down, 0.28f, nullptr, &main);

    ImGui::DockBuilderDockWindow("LCD", main);
    ImGui::DockBuilderDockWindow("Logs", bottom);
    ImGui::DockBuilderDockWindow("Input", left);
    ImGui::DockBuilderDockWindow("TileMap 2", center_top);
    ImGui::DockBuilderDockWindow("TileMap 1", center_top);
    ImGui::DockBuilderDockWindow("Tile Data", center_top);
    ImGui::DockBuilderDockWindow("Sprites", center_bottom);
    ImGui::DockBuilderDockWindow("Memory", right);
    ImGui::DockBuilderDockWindow("Registers", right_center);
    ImGui::DockBuilderDockWindow("Instructions", right_bottom);
    ImGui::DockBuilderFinish(dockspace_id);
  }
}

void Interface::RenderError() {
  ZoneScoped;
  error_messages_.Draw();
}

void Interface::LoadCartridge() {
  nfdchar_t* file_path = nullptr;
  std::array<nfdfilteritem_t, 3> filter_items = {{
    { .name="GB", .spec="gb" },
    { .name="BIN", .spec="bin" },
    { .name="ROM", .spec="rom" }
  }};

  nfdresult_t result = NFD_OpenDialog(&file_path, filter_items.data(), filter_items.size(), nullptr);
  if (result == NFD_OKAY) {
    LoadCartRom(file_path);
  } else if (result == NFD_CANCEL) {
    spdlog::info("Load cancelled by user.");
  } else {
    spdlog::error("Loading failed: {}", NFD_GetError());
  }
}

void Interface::LoadCartRom(std::string_view file_path) {
  Stop();

  fs::path path { file_path };
  auto ext = path.extension();
  if (ext != ".gb" && ext != ".bin" && ext != ".rom") {
    config_.settings.recent_files.Remove(path);
    spdlog::error("Invalid file extension: {}. Only supports loading .gb, .bin, .rom", ext.string());
    return;
  }

  auto load_result = File::LoadBin(path.string());
  if (!load_result) {
    config_.settings.recent_files.Remove(path);
    std::string error = std::format("Failed to load cart: {}", load_result.error());
    spdlog::error("Failed to load cart: {}", error);
    error_messages_.AddError(kCartRomErrorKey, error);
    return;
  } else {
    error_messages_.ClearError(kCartRomErrorKey);
  }

  emulator_.LoadCartBytes(std::move(load_result.value()));
  spdlog::info("Loaded cartridge: '{}'", fs::absolute(path).string());

  fs::path rom_path{path};
  SetWindowTitle(std::format("{} - {}", kWindowTitle, rom_path.stem().string()).c_str());

  emulator_.ToggleChannel(AudioChannelID::MASTER, config_.settings.enable_audio);

  if (config_.settings.auto_start) {
    Play();
  }

  config_.settings.recent_files.Push(path.string());
}

void Interface::RenderLCD() {
  ZoneScoped;

  if (!config_.settings.show_lcd) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  ImGui::Begin("LCD", &config_.settings.show_lcd);
  {
    rlImGuiImageTextureFit(&emulator_.GetTargetLCD(), true);
  }
  ImGui::End();
}

void Interface::RenderTiles() {
  ZoneScoped;

  if (!config_.settings.show_tiles) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Tile Data", &config_.settings.show_tiles)) {
    auto& target = emulator_.GetTargetTiles();
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 3;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::RenderTilemap1() {
  ZoneScoped;

  if (!config_.settings.show_tilemap1) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("TileMap 1", &config_.settings.show_tilemap1)) {
    auto& target = emulator_.GetTargetTilemap(0);
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::RenderTilemap2() {
  ZoneScoped;

  if (!config_.settings.show_tilemap2) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("TileMap 2", &config_.settings.show_tilemap2)) {
    auto& target = emulator_.GetTargetTilemap(1);
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::RenderSprites() {
  ZoneScoped;

  if (!config_.settings.show_sprites) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Sprites", &config_.settings.show_sprites)) {
    auto& target = emulator_.GetTargetSprites();
    auto width = target.texture.width;
    auto height = target.texture.height;
    auto scale = 2;
    rlImGuiImageRect(&target.texture, width * scale, height * scale, Rectangle { 0, 0, static_cast<float>(width), -static_cast<float>(height) });
  }
  ImGui::End();
}

void Interface::RenderRegisters() {
  ZoneScoped;

  if (!config_.settings.show_cpu_registers) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Registers", &config_.settings.show_cpu_registers)) {
    const auto& regs = emulator_.GetRegisters();

    auto a = regs.Get(Reg8::A);
    auto f = regs.Get(Reg8::F);
    auto b = regs.Get(Reg8::B);
    auto c = regs.Get(Reg8::C);
    auto d = regs.Get(Reg8::D);
    auto e = regs.Get(Reg8::E);
    auto h = regs.Get(Reg8::H);
    auto l = regs.Get(Reg8::L);
    auto af = regs.Get(Reg16::AF);
    auto bc = regs.Get(Reg16::BC);
    auto de = regs.Get(Reg16::DE);
    auto hl = regs.Get(Reg16::HL);
    auto zero = regs.Get(Flag::Z);
    auto neg = regs.Get(Flag::N);
    auto half_carry = regs.Get(Flag::H);
    auto carry = regs.Get(Flag::C);

    ImGui::Text("PC=%04X (%d)", regs.pc, regs.pc);
    ImGui::Text("SP=%04X (%d)", regs.sp, regs.sp);
    ImGui::Text("A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X", a, f, b, c, d, e, h, l);
    ImGui::Text("AF=%04X BC=%04X DE=%04X HL=%04X", af, bc, de, hl);
    ImGui::Text("Flags Z=%d N=%d H=%d C=%d", zero, neg, half_carry, carry);

    const auto& state = emulator_.GetState();
    ImGui::Text("State IME=%d HALT=%d STOP=%d HARD_LOCK=%d", state.ime, state.halt, state.stop, state.hard_lock);
  }
  ImGui::End();
}

void Interface::RenderInput() {
  ZoneScoped;

  if (!config_.settings.show_input) {
    return;
  }

  // ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Input", &config_.settings.show_input)) {

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
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Up), []() {
      ImGui::Button(ICON_FA_CARET_UP, { 32, 32 });
    });

    ImGui::SameLine(0, 32);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Select), []() {
      ImGui::Button("select");
    });
    ImGui::SameLine(0, 4);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Start), []() {
      ImGui::Button("start");
    });
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::NewLine();
    alignForWidth(288);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Left), []() {
      ImGui::Button(ICON_FA_CARET_LEFT, { 32, 32 });
    });
    ImGui::SameLine(0, 32);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Right), []() {
      ImGui::Button(ICON_FA_CARET_RIGHT, { 32, 32 });
    });

    ImGui::SameLine(0, 64);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 24.0f);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::B), []() {
      ImGui::Button("B", { 32, 32 });
    });
    ImGui::SameLine();
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::A), []() {
      ImGui::Button("A", { 32, 32 });
    });
    ImGui::PopStyleVar();

    ImGui::NewLine();
    alignForWidth(288);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
    drawMaybeDisabled(emulator_.IsButtonPressed(JoypadButton::Down), []() {
      ImGui::Button(ICON_FA_CARET_DOWN, { 32, 32 });
    });

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
  }
  ImGui::End();
}

void Interface::RenderMemory() {
  ZoneScoped;

  if (!config_.settings.show_memory) {
    return;
  }

  // ImGui::SetNextWindowSize(ImVec2{300, 300}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Memory", &config_.settings.show_memory)) {
    mem_editor_.DrawContents(nullptr, 1<<16);
  }
  ImGui::End();
}

void Interface::RenderInstructions() {
  ZoneScoped;

  if (!config_.settings.show_instructions) {
    return;
  }

  // ImGui::SetNextWindowSize(ImVec2{300, 300}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Instructions", &config_.settings.show_instructions)) {
    assembly_viewer_.Draw();
  }
  ImGui::End();
}

void Interface::RenderLogs() {
  ZoneScoped;

  if (!config_.settings.show_logs) {
    return;
  }

  if (ImGui::Begin("Logs", &config_.settings.show_logs)) {
    app_log_.Draw();
  }
  ImGui::End();
}

void Interface::RenderMainMenu() {
  ZoneScoped;

  ImGui::BeginMainMenuBar();

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Load Cartridge")) {
      spdlog::info("Loading cart...");
      LoadCartridge();
    }

    auto& recent_files = config_.settings.recent_files;
    if (ImGui::BeginMenu("Recent files", !recent_files.IsEmpty())) {
      for (auto& file : recent_files) {
        if (ImGui::MenuItem(fs::relative(file).c_str())) {
          spdlog::debug("Pressed recent file: '{}'", file);
          LoadCartRom(file);
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
    if (ImGui::MenuItem("Settings...")) {
      spdlog::debug("Open settings...");
      show_settings_ = true;
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Exit")) {
      spdlog::info("Exiting...");
      should_close_ = true;
    }

    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Emulator")) {
    ImGui::MenuItem("Auto-Start", nullptr, &config_.settings.auto_start);
    if (ImGui::MenuItem("Skip Boot ROM", nullptr, &config_.settings.skip_boot_rom)) {
      emulator_.SetSkipBootRom(config_.settings.skip_boot_rom);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Play", nullptr, nullptr, !emulator_.IsPlaying())) {
      Play();
    }
    if (ImGui::MenuItem("Stop", nullptr, nullptr, emulator_.IsPlaying())) {
      Stop();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Step", nullptr, nullptr, !emulator_.IsPlaying())) {
      Step();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Reset")) {
      Reset();
      if (config_.settings.auto_start) {
        Play();
      }
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Hardware")) {
    if (ImGui::BeginMenu("PPU")) {
      ImGui::MenuItem("LCD", nullptr, &config_.settings.show_lcd);
      ImGui::MenuItem("TileMap 1", nullptr, &config_.settings.show_tilemap1);
      ImGui::MenuItem("TileMap 2", nullptr, &config_.settings.show_tilemap2);
      ImGui::MenuItem("Sprites", nullptr, &config_.settings.show_sprites);
      ImGui::MenuItem("Tiles", nullptr, &config_.settings.show_tiles);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("APU")) {
      if (ImGui::MenuItem("Enable Sound", nullptr, &config_.settings.enable_audio)) {
        emulator_.ToggleChannel(AudioChannelID::MASTER, config_.settings.enable_audio);
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
        if (ImGui::MenuItem("CH1 - Square", nullptr, &config_.settings.enable_ch1)) {
          emulator_.ToggleChannel(AudioChannelID::CH1, config_.settings.enable_ch1);
        }
        if (ImGui::MenuItem("CH2 - Square", nullptr, &config_.settings.enable_ch2)) {
          emulator_.ToggleChannel(AudioChannelID::CH2, config_.settings.enable_ch2);
        }
        if (ImGui::MenuItem("CH3 - Wave", nullptr, &config_.settings.enable_ch3)) {
          emulator_.ToggleChannel(AudioChannelID::CH3, config_.settings.enable_ch3);
        }
        if (ImGui::MenuItem("CH4 - Noise", nullptr, &config_.settings.enable_ch4)) {
          emulator_.ToggleChannel(AudioChannelID::CH4, config_.settings.enable_ch4);
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::Separator();
    ImGui::MenuItem("Memory", nullptr, &config_.settings.show_memory);
    ImGui::MenuItem("Registers", nullptr, &config_.settings.show_cpu_registers);
    ImGui::MenuItem("Instructions", nullptr, &config_.settings.show_instructions);
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Input", nullptr, &config_.settings.show_input);
    ImGui::MenuItem("Logs", nullptr, &config_.settings.show_logs);
    ImGui::Separator();
    if (ImGui::MenuItem("Lock FrameRate", nullptr, &config_.settings.lock_framerate)) {
      if (config_.settings.lock_framerate) {
        SetTargetFPS(kLockedFrameRate);
      } else {
        SetTargetFPS(0);
      }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Reset View")) {
      ResetView();
    }
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();
}

void Interface::RenderSettingsPopup() {
  ZoneScoped;

  if (show_settings_) {
    ImGui::OpenPopup("Settings");
    show_settings_ = false;
  }

  if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_Modal | ImGuiWindowFlags_AlwaysAutoResize)) {
    static std::string boot_rom_path = config_.settings.boot_rom_path;
    ImGui::InputText("Boot ROM Path", &boot_rom_path);

    if (ImGui::Button("OK", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
      spdlog::debug("Set boot rom path: {}", boot_rom_path);
      config_.settings.boot_rom_path = boot_rom_path;
      if (auto result = emulator_.SetBootRomPath(config_.settings.boot_rom_path); !result) {
        error_messages_.AddError(kBootRomErrorKey, std::format("Failed to load boot rom: {}", result.error()));
      } else {
        error_messages_.ClearError(kBootRomErrorKey);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
      boot_rom_path = config_.settings.boot_rom_path;
    }

    ImGui::EndPopup();
  }
}

void Interface::RenderStatusBar() {
  if (ImGui::BeginViewportSideBar("###MainStatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(), ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
    if (ImGui::BeginMenuBar()) {
      {
        ImVec4 color{0.0f, 0.617f, 0.184f, 1.0f}; // LIME
        int fps = GetFPS();

        if ((fps < 30) && (fps >= 15)) {
          color = ImVec4{1.0f, 0.631f, 0.0f, 1.0f}; // ORANGE
        } else if (fps < 15) {
          color = ImVec4{0.902f, 0.160f, 0.216f, 1.0f}; // RED
        }

        ImGui::TextColored(color, "FPS: %3d", fps);
      }
      {
        const float frame_time = GetFrameTime();
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
        ImGui::Text("FrameTime: %0.4fms", frame_time);
      }
      {
        size_t cycles = emulator_.GetPrevCycles();
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
        ImGui::Text("Cycles/Update: %3lu", cycles);
      }
      {
        static double last_update = 0.0;
        static size_t gb_frames = 0;
        double current_time = GetTime();
        double delta_time = current_time - last_update;

        if (delta_time >= 1.0) {
          last_update = current_time;
          gb_frames = emulator_.GetFrameCount();
          emulator_.ResetFrameCount();
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 32);
        ImGui::Text("GB FrameRate: %3lu", gb_frames);
      }
      ImGui::EndMenuBar();
    }
    ImGui::End();
  }
}

void Interface::Cleanup() {
  auto window_pos = GetWindowPosition();
  config_.settings.screen_x = static_cast<int>(window_pos.x);
  config_.settings.screen_y = static_cast<int>(window_pos.y);
  config_.settings.master_volume = GetMasterVolume() * 100.0f;
  config_.settings.boot_rom_path = emulator_.GetBootRomPath();

  if (auto res = config_.Save(args_.settings_filename); !res.has_value()) {
    spdlog::warn("Failed to save settings to file '{}': {}", args_.settings_filename, res.error());
  }
}

void Interface::Play() {
  emulator_.Play();
  PlayAudioStream(stream);
}

void Interface::Stop() {
  emulator_.Stop();
  StopAudioStream(stream);
}

void Interface::Step() {
  if (!emulator_.IsPlaying()) {
    emulator_.Step();
  }
}

void Interface::Reset() {
  emulator_.Reset();
  StopAudioStream(stream);
}

void Interface::ResetView() {
  config_.settings.reset_view = true;
  config_.settings.show_lcd = true;
  config_.settings.show_tiles = true;
  config_.settings.show_tilemap1 = true;
  config_.settings.show_tilemap2 = true;
  config_.settings.show_sprites = true;
  config_.settings.show_cpu_registers = true;
  config_.settings.show_input = true;
  config_.settings.show_memory = true;
  config_.settings.show_instructions = true;
  config_.settings.show_logs = true;
}
