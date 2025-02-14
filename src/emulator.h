#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <tl/expected.hpp>

#include "cpu.h"
#include "ppu.h"
#include "registers.h"
#include "boot_rom_device.h"
#include "cart_device.h"
#include "wram_device.h"
#include "hram_device.h"
#include "timer.h"
#include "audio.h"
#include "null_device.h"
#include "input_device.h"
#include "serial_device.h"

class Emulator {
public:
  explicit Emulator(bool doctor_log);
  ~Emulator() = default;

  tl::expected<bool, std::string> init();
  void cleanup();
  void update();
  void reset();
  void step();
  void play();
  void stop();

  void load_cartridge(std::vector<uint8_t> &&bytes);
  [[nodiscard]] bool is_playing() const;
  void set_skip_bootrom(bool skip);
  [[nodiscard]] bool skip_bootrom() const;

  [[nodiscard]] const Registers& registers() const;
  [[nodiscard]] const State& state() const;
  [[nodiscard]] size_t cycles() const;
  [[nodiscard]] PPUMode mode() const;
  [[nodiscard]] Instruction instr() const;
  [[nodiscard]] uint8_t read8(uint16_t addr) const;

  [[nodiscard]] const Texture2D& target_lcd() const;
  [[nodiscard]] const RenderTexture2D& target_tiles() const;
  [[nodiscard]] const RenderTexture2D& target_tilemap(uint8_t id) const;
  [[nodiscard]] const RenderTexture2D& target_sprites() const;

  void add_breakpoint(uint16_t addr);
  void remove_breakpoint(uint16_t addr);
  void clear_breakpoints();

  void update_input(JoypadButton btn, bool pressed);

  void log_doctor() const;

private:
  Mmu mmu;
  Cpu cpu;
  Ppu ppu;
  BootRomDevice boot;
  CartDevice cart;
  WramDevice wram;
  HramDevice hram;
  Timer timer;
  InterruptDevice interrupts;
  Audio audio;
  NullDevice null_device;
  InputDevice input_device;
  SerialDevice serial_device;

  std::array<uint8_t, kBootRomSize> boot_rom;
  std::vector<uint8_t> cart_bytes;
  std::set<uint16_t> breakpoints;

  size_t num_cycles = 0;
  bool running = false;
  bool _skip_bootrom = true;
  bool _doctor_log = false;
};
