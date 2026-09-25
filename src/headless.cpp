#include <atomic>
#include <cstdio>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <raylib.h>

#include "command_parser.hpp"
#include "file.hpp"
#include "headless.hpp"

using namespace app;

namespace fs = std::filesystem;

namespace {
  constexpr size_t kDmgClockSpeed = 4194304;
  constexpr size_t kGbcClockSpeed = 8388608;
  constexpr int kAudioSampleRate = 44100;
  constexpr int kAudioSampleSize = 32;
  constexpr int kAudioNumChannels = 2;
  constexpr int kSamplesPerUpdate = 512;
  constexpr float kFrameRate = 59.73;

  constexpr std::array<Color, 4> kDefaultPalette {
    Color { 223, 247, 207, 255 },
    Color { 135, 192, 111, 255 },
    Color { 51, 104, 85, 255 },
    Color { 8, 23, 32, 255 },
  };

  std::atomic<bool> g_should_quit = false;
  bool g_interactive = false;

  struct StreamDeleter {
    void operator()(std::istream* stream) const {
      if (stream == &std::cin) {
        return;
      }
      delete stream;
    }
  };

  std::unique_ptr<std::istream, StreamDeleter> g_istream;

  bool IsInteractive() {
    return isatty(fileno(stdin));
  }

  void SignalHandler(int signal) {
    if (signal == SIGINT) {
      spdlog::info("Quit requested...");
      g_should_quit = true;
    }
  }

  void LoadCommand(const Command& command, Emulator& emulator) {
    const auto& path = command.path;
    spdlog::trace("Loading rom at '{}'", path);
    auto load_result = file::LoadBin(path);
    if (!load_result) {
      std::string error = std::format("Failed to load cart: {}", load_result.error());
      spdlog::error("{}", error);
      return;
    }

    emulator.LoadCartBytes(std::move(load_result.value()));
    spdlog::info("Loaded cartridge: '{}'", fs::absolute(path).string());
  }

  void ResetCommand(const Command& command, Emulator& emulator) {
    emulator.Reset();
  }

  void StepCommand(const Command& command, Emulator& emulator) {
    int steps = command.steps;
    while (steps-- > 0) {
      emulator.Step();
    }
  }

  void WriteCommand(const Command& command, Emulator& emulator) {
    emulator.Write8(command.address, command.value);
    std::println("Write @{:04X} = {:02X}", command.address, command.value);
  }

  void ReadCommand(const Command& command, Emulator& emulator) {
    auto byte = emulator.Read8(command.address);
    std::println("Read @{:04X} = {:02X}", command.address, byte);
  }
}

void Headless::Init(Args args) {
  std::signal(SIGINT, SignalHandler);

  spdlog::info("Headless mode activated");

  if (!args.script.empty()) {
    g_interactive = false;
    g_istream = std::unique_ptr<std::istream, StreamDeleter>(new std::ifstream(args.script));
    spdlog::info("Loading script at '{}'", args.script);
  } else {
    g_istream = std::unique_ptr<std::istream, StreamDeleter>(&std::cin);
    g_interactive = IsInteractive();
    if (g_interactive) {
      spdlog::info("Interactive mode");
    }
  }

  EmulatorConfig emu_cfg{
    .palette = kDefaultPalette,
    .clock_speed = kDmgClockSpeed,
    .sample_rate = kAudioSampleRate,
    .buffer_size = kSamplesPerUpdate,
    .num_channels = kAudioNumChannels,
    .frame_rate = kFrameRate,
  };

  emulator_.Init(emu_cfg);
  emulator_.SetSkipBootRom(true);
}

void Headless::Cleanup() {
  spdlog::info("Cleaning up.");
}

void Headless::Eval(const Command& command) {
  switch (command.type) {
    case CommandType::Unknown: spdlog::warn("Unknown command: '{}'", command.line); break;
    case CommandType::Load: LoadCommand(command, emulator_); break;
    case CommandType::Reset: ResetCommand(command, emulator_); break;
    case CommandType::Step: StepCommand(command, emulator_); break;
    case CommandType::Write: WriteCommand(command, emulator_); break;
    case CommandType::Read: ReadCommand(command, emulator_); break;
    default: std::unreachable();
  }
}

int Headless::Run() {
  if (g_istream->fail()) {
    spdlog::error("Failed to open script");
    return 1;
  }

  Command command;
  std::string line;

  while (!g_should_quit) {
    if (g_interactive) {
      std::print("> ");
    }

    if (!std::getline(*g_istream.get(), line)) {
      spdlog::info("Quitting...");
      break;
    }

    std::print("entered: {}", line);

    if (line.empty()) {
      continue;
    }

    command = CommandParser::Parse(line);
    Eval(command);
  }

  return 0;
}
