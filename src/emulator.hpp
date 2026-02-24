#pragma once

#include <array>
#include <expected>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "types.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "registers.hpp"
#include "boot_rom_device.hpp"
#include "cart_device.hpp"
#include "wram_device.hpp"
#include "hram_device.hpp"
#include "timer.hpp"
#include "audio.hpp"
#include "null_device.h"
#include "input_device.hpp"
#include "serial_device.hpp"


class Emulator {
public:
  explicit Emulator(audio_config cfg);
  Emulator() = delete;
  Emulator(const Emulator&) = delete;
  ~Emulator() = default;

  void Init();
  void Cleanup();
  void Update(float dt);
  void Reset();
  void Step();
  void Play();
  void Stop();

  void LoadCartBytes(std::vector<u8> bytes);
  [[nodiscard]] bool IsPlaying() const;
  void SetSkipBootRom(bool skip);
  [[nodiscard]] bool ShouldSkipBootRom() const;

  [[nodiscard]] const Registers& GetRegisters() const;
  [[nodiscard]] const State& GetState() const;
  [[nodiscard]] size_t GetTotalCycles() const;
  [[nodiscard]] PPUMode GetMode() const;
  [[nodiscard]] Instruction GetCurrentInstruction() const;
  [[nodiscard]] u8 Read8(u16 addr) const;
  [[nodiscard]] u16 Read16(u16 addr) const;
  void Write8(u16 addr, u8 byte);

  [[nodiscard]] const Texture2D& GetTargetLCD() const;
  [[nodiscard]] const RenderTexture2D& GetTargetTiles() const;
  [[nodiscard]] const RenderTexture2D& GetTargetTilemap(u8 id) const;
  [[nodiscard]] const RenderTexture2D& GetTargetSprites() const;

  void AddBreakPoint(u16 addr);
  void RemoveBreakPoint(u16 addr);
  void ClearBreakPoints();

  void UpdateInput(JoypadButton btn, bool pressed);
  bool IsButtonPressed(JoypadButton btn) const;

  void ToggleChannel(AudioChannelID channel, bool enable);
  bool IsChannelEnabled(AudioChannelID channel) const;
  std::vector<float>& GetAudioSamples();
  void OnAudioCallback(std::span<float> buffer);

  std::expected<void, std::string> SetBootRomPath(std::string_view path);
  std::string GetBootRomPath() const;

  size_t GetPrevCycles() const;

  void ResetFrameCount();
  size_t GetFrameCount() const;

private:
  Mmu mmu_;
  Cpu cpu_;
  Ppu ppu_;
  BootRomDevice boot_;
  CartDevice cart_;
  WramDevice wram_;
  HramDevice hram_;
  Timer timer_;
  InterruptDevice interrupts_;
  Audio audio_;
  NullDevice null_device_;
  InputDevice input_device_;
  SerialDevice serial_device_;

  std::array<u8, kBootRomSize> boot_rom_;
  std::vector<u8> cart_bytes_;
  std::set<u16> breakpoints_;

  std::vector<float> sample_bufffer_ {};

  size_t prev_cycles_ = 0;
  size_t num_cycles_ = 0;
  bool running_ = false;

  std::string boot_rom_path_;
  bool skip_bootrom_ = true;
};
