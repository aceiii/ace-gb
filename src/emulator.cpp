#include <array>
#include <expected>
#include <format>
#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.hpp"
#include "file.hpp"
#include "mmu.hpp"

// NOTE: not sure why, but my emulator seems to be 4x slower than it should be...
constexpr auto kStaticSpeedMultiplier = 4;

Emulator::Emulator(audio_config cfg):cpu_{mmu_, interrupts_}, ppu_{mmu_, interrupts_}, serial_device_{interrupts_}, timer_{interrupts_}, input_device_{interrupts_}, audio_{timer_, cfg} {
  serial_device_.on_line([] (const std::string &str) {
    spdlog::info("Serial: {}", str);
  });

  sample_bufffer_.resize(cfg.num_channels * cfg.buffer_size);
}

void Emulator::Init() {
  ppu_.Init();

  mmu_.ClearDevices();
  mmu_.AddDevice(&boot_);
  mmu_.AddDevice(&cart_);
  mmu_.AddDevice(&wram_);
  mmu_.AddDevice(&ppu_);
  mmu_.AddDevice(&hram_);
  mmu_.AddDevice(&audio_);
  mmu_.AddDevice(&timer_);
  mmu_.AddDevice(&input_device_);
  mmu_.AddDevice(&serial_device_);
  mmu_.AddDevice(&interrupts_);
  mmu_.AddDevice(&null_device_);

  cpu_.AddSyncedDevice(&timer_);
  cpu_.AddSyncedDevice(&ppu_);
  cpu_.AddSyncedDevice(&audio_);
  cpu_.AddSyncedDevice(&serial_device_);
}

void Emulator::Update(float dt) {
  if (!running_ && !cpu_.state.halt) {
    return;
  }

  static constexpr auto target_cycles_per_frame = static_cast<int>(kClockSpeed / kFrameRate);
  static int current_cycles = 0;

  do {
    auto cycles = cpu_.Execute();
    current_cycles += cycles;
    num_cycles_ += cycles;

    if (breakpoints_.contains(cpu_.regs.pc)) {
      running_ = false;
      break;
    }
  } while (current_cycles < target_cycles_per_frame);
  current_cycles -= target_cycles_per_frame;

  ppu_.UpdateRenderTargets();
}

void Emulator::Cleanup() {
  ppu_.Cleanup();
}

void Emulator::LoadCartBytes(std::vector<uint8_t> &&bytes) {
  cart_bytes_ = std::move(bytes);
  Reset();
}

void Emulator::Reset() {
  num_cycles_ = 0;
  running_ = false;

  cpu_.Reset();
  mmu_.ResetDevices();
  boot_.LoadBytes(boot_rom_);
  cart_.LoadCartBytes(cart_bytes_);

  if (skip_bootrom_) {
    cpu_.regs.Set(Reg8::A, 0x01);
    cpu_.regs.Set(Reg8::F, 0xb0);
    cpu_.regs.Set(Reg8::B, 0x00);
    cpu_.regs.Set(Reg8::C, 0x13);
    cpu_.regs.Set(Reg8::D, 0x00);
    cpu_.regs.Set(Reg8::E, 0xd8);
    cpu_.regs.Set(Reg8::H, 0x01);
    cpu_.regs.Set(Reg8::L, 0x4d);
    cpu_.regs.sp = 0xfffe;
    cpu_.regs.pc = 0x0100;

    mmu_.Write8(std::to_underlying(IO::P1), 0xcf);
    mmu_.Write8(std::to_underlying(IO::SB), 0x00);
    mmu_.Write8(std::to_underlying(IO::SC), 0x7e);
    mmu_.Write8(std::to_underlying(IO::DIV), 0xab);
    mmu_.Write8(std::to_underlying(IO::TIMA), 0x00);
    mmu_.Write8(std::to_underlying(IO::TMA), 0x00);
    mmu_.Write8(std::to_underlying(IO::TAC), 0xf8);
    mmu_.Write8(std::to_underlying(IO::IF), 0xE1);
    mmu_.Write8(std::to_underlying(IO::NR10), 0x80);
    mmu_.Write8(std::to_underlying(IO::NR11), 0xbf);
    mmu_.Write8(std::to_underlying(IO::NR12), 0xf3);
    mmu_.Write8(std::to_underlying(IO::NR13), 0xff);
    mmu_.Write8(std::to_underlying(IO::NR14), 0xbf);
    mmu_.Write8(std::to_underlying(IO::NR21), 0x3f);
    mmu_.Write8(std::to_underlying(IO::NR22), 0x00);
    mmu_.Write8(std::to_underlying(IO::NR23), 0xff);
    mmu_.Write8(std::to_underlying(IO::NR24), 0xbf);
    mmu_.Write8(std::to_underlying(IO::NR30), 0x7f);
    mmu_.Write8(std::to_underlying(IO::NR31), 0xff);
    mmu_.Write8(std::to_underlying(IO::NR32), 0x9f);
    mmu_.Write8(std::to_underlying(IO::NR33), 0xff);
    mmu_.Write8(std::to_underlying(IO::NR34), 0xbf);
    mmu_.Write8(std::to_underlying(IO::NR41), 0xff);
    mmu_.Write8(std::to_underlying(IO::NR42), 0x00);
    mmu_.Write8(std::to_underlying(IO::NR43), 0x00);
    mmu_.Write8(std::to_underlying(IO::NR44), 0xbf);
    mmu_.Write8(std::to_underlying(IO::NR50), 0x77);
    mmu_.Write8(std::to_underlying(IO::NR51), 0xf3);
    mmu_.Write8(std::to_underlying(IO::NR52), 0xf0);
    mmu_.Write8(std::to_underlying(IO::LCDC), 0x91);
    mmu_.Write8(std::to_underlying(IO::STAT), 0x85);
    mmu_.Write8(std::to_underlying(IO::LYC), 0x00);
    mmu_.Write8(std::to_underlying(IO::DMA), 0xff);
    mmu_.Write8(std::to_underlying(IO::BGP), 0xfc);
    mmu_.Write8(std::to_underlying(IO::WX), 0x00);
    mmu_.Write8(std::to_underlying(IO::WY), 0x00);
    mmu_.Write8(std::to_underlying(IO::IE), 0x00);

    boot_.SetDisable(0xff);
  }
}

void Emulator::Step() {
  if (running_) {
    return;
  }
  num_cycles_ += cpu_.Execute();
}

void Emulator::Play() {
  running_ = true;
}

void Emulator::Stop() {
  running_ = false;
}

bool Emulator::IsPlaying() const {
  return running_;
}

const Registers& Emulator::GetRegisters() const {
  return cpu_.regs;
}

const State& Emulator::GetState() const {
  return cpu_.state;
}

size_t Emulator::GetCycles() const {
  return num_cycles_;
}

PPUMode Emulator::GetMode() const {
  return ppu_.GetMode();
}

Instruction Emulator::GetCurrentInstruction() const {
  auto byte = mmu_.Read8(cpu_.regs.pc);
  auto instr = Decoder::Decode(byte);
  if (instr.opcode == Opcode::PREFIX) {
    byte = mmu_.Read8(cpu_.regs.pc + 1);
    instr = Decoder::DecodePrefixed(byte);
  }
  return instr;
}

uint8_t Emulator::Read8(uint16_t addr) const {
  return mmu_.Read8(addr);
}

void Emulator::write8(uint16_t addr, uint8_t byte) {
  mmu_.Write8(addr, byte);
}

const Texture2D& Emulator::GetTargetLCD() const {
  return ppu_.GetTextureLcd();
}

const RenderTexture2D& Emulator::GetTargetTiles() const {
  return ppu_.GetTextureTiles();
}

const RenderTexture2D& Emulator::GetTargetTilemap(uint8_t idx) const {
  if (idx == 0) {
    return ppu_.GetTextureTilemap1();
  } else if (idx == 1) {
    return ppu_.GetTextureTilemap2();
  }
  std::unreachable();
}

const RenderTexture2D& Emulator::GetTargetSprites() const {
  return ppu_.GetTextureSprites();
}

void Emulator::AddBreakPoint(uint16_t addr) {
  breakpoints_.insert(addr);
}

void Emulator::RemoveBreakPoint(uint16_t addr) {
  breakpoints_.erase(addr);
}

void Emulator::ClearBreakPoints() {
  breakpoints_.clear();
}

void Emulator::UpdateInput(JoypadButton btn, bool pressed) {
  input_device_.Update(btn, pressed);
}

bool Emulator::IsButtonPressed(JoypadButton btn) const {
  return input_device_.IsPressed(btn);
}

void Emulator::SetSkipBootRom(bool skip) {
  skip_bootrom_ = skip;
}

bool Emulator::ShouldSkipBootRom() const {
  return skip_bootrom_;
}

void Emulator::ToggleChannel(AudioChannelID channel, bool enable) {
  audio_.ToggleChannel(channel, enable);
}

bool Emulator::IsChannelEnabled(AudioChannelID channel) const {
  return audio_.IsChannelEnabled(channel);
}

std::vector<float>& Emulator::GetAudioSamples() {
  audio_.GetSamples(sample_bufffer_);
  return sample_bufffer_;
}

std::expected<void, std::string> Emulator::SetBootRomPath(std::string_view path) {
  boot_rom_.fill(0);
  boot_rom_path_ = path;

  auto result = File::LoadBin(boot_rom_path_);
  if (!result) {
    return std::unexpected{result.error()};
  }

  const auto &bytes = result.value();
  std::copy_n(bytes.begin(), std::min(bytes.size(), boot_rom_.size()), boot_rom_.begin());

  return {};
}

std::string Emulator::GetBootRomPath() const {
  return boot_rom_path_;
}
