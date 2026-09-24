#include <atomic>
#include <cstdio>
#include <csignal>
#include <iostream>
#include <print>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "command_parser.hpp"
#include "headless.hpp"

using namespace app;

namespace {
  std::atomic<bool> g_should_quit = false;
  bool g_interactive = false;

  bool IsInteractive() {
    return isatty(fileno(stdin));
  }

  void SignalHandler(int signal) {
    if (signal == SIGINT) {
      spdlog::info("Quit requested...");
      g_should_quit = true;
    }
  }
}

void Headless::Init(Args args) {
  std::signal(SIGINT, SignalHandler);

  spdlog::info("Headless mode activated");

  g_interactive = IsInteractive();
  if (g_interactive) {
    spdlog::info("Interactive mode");
  }
}

void Headless::Cleanup() {
  spdlog::info("Cleaning up.");
}

void Headless::Eval(const Command& command) {
  if (command.type == CommandType::Unknown) {
    spdlog::warn("Unknown command: '{}', {}", command.line, command.message);
    return;
  }
}

void Headless::Run() {
  Command command;
  std::string line;

  while (!g_should_quit) {
    if (g_interactive) {
      std::print("> ");
    }

    if (!std::getline(std::cin, line)) {
      spdlog::info("Quitting...");
      break;
    }

    command = CommandParser::Parse(line);
    Eval(command);

  }
}
