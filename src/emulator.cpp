#include <array>
#include <expected>
#include <format>
#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>
#include <tracy/Tracy.hpp>

#include "types.hpp"
#include "emulator.hpp"
#include "file.hpp"
#include "mmu.hpp"


namespace {
  constexpr int kMinBootRomSize = 256;
};

static void SetDmgBootRegisters(Mmu& mmu, Registers& regs) {
    regs.Set(Reg8::A, 0x01);
    regs.Set(Reg8::F, 0xb0);
    regs.Set(Reg8::B, 0x00);
    regs.Set(Reg8::C, 0x13);
    regs.Set(Reg8::D, 0x00);
    regs.Set(Reg8::E, 0xd8);
    regs.Set(Reg8::H, 0x01);
    regs.Set(Reg8::L, 0x4d);
    regs.sp = 0xfffe;
    regs.pc = 0x0100;

    mmu.Write8(std::to_underlying(IO::P1), 0xcf);
    mmu.Write8(std::to_underlying(IO::SB), 0x00);
    mmu.Write8(std::to_underlying(IO::SC), 0x7e);
    mmu.Write8(std::to_underlying(IO::DIV), 0xab);
    mmu.Write8(std::to_underlying(IO::TIMA), 0x00);
    mmu.Write8(std::to_underlying(IO::TMA), 0x00);
    mmu.Write8(std::to_underlying(IO::TAC), 0xf8);
    mmu.Write8(std::to_underlying(IO::IF), 0xE1);
    mmu.Write8(std::to_underlying(IO::NR10), 0x80);
    mmu.Write8(std::to_underlying(IO::NR11), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR12), 0xf3);
    mmu.Write8(std::to_underlying(IO::NR13), 0xff);
    mmu.Write8(std::to_underlying(IO::NR14), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR21), 0x3f);
    mmu.Write8(std::to_underlying(IO::NR22), 0x00);
    mmu.Write8(std::to_underlying(IO::NR23), 0xff);
    mmu.Write8(std::to_underlying(IO::NR24), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR30), 0x7f);
    mmu.Write8(std::to_underlying(IO::NR31), 0xff);
    mmu.Write8(std::to_underlying(IO::NR32), 0x9f);
    mmu.Write8(std::to_underlying(IO::NR33), 0xff);
    mmu.Write8(std::to_underlying(IO::NR34), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR41), 0xff);
    mmu.Write8(std::to_underlying(IO::NR42), 0x00);
    mmu.Write8(std::to_underlying(IO::NR43), 0x00);
    mmu.Write8(std::to_underlying(IO::NR44), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR50), 0x77);
    mmu.Write8(std::to_underlying(IO::NR51), 0xf3);
    mmu.Write8(std::to_underlying(IO::NR52), 0xf0);
    mmu.Write8(std::to_underlying(IO::LCDC), 0x91);
    mmu.Write8(std::to_underlying(IO::STAT), 0x85);
    mmu.Write8(std::to_underlying(IO::SCX), 0x00);
    mmu.Write8(std::to_underlying(IO::SCY), 0x00);
    mmu.Write8(std::to_underlying(IO::LY), 0x91);
    mmu.Write8(std::to_underlying(IO::LYC), 0x00);
    mmu.Write8(std::to_underlying(IO::DMA), 0xff);
    mmu.Write8(std::to_underlying(IO::BGP), 0xfc);
    mmu.Write8(std::to_underlying(IO::OBP0), 0x00);
    mmu.Write8(std::to_underlying(IO::OBP1), 0x00);
    mmu.Write8(std::to_underlying(IO::WY), 0x00);
    mmu.Write8(std::to_underlying(IO::WX), 0x00);
    mmu.Write8(std::to_underlying(IO::IE), 0x00);
}

static void SetCgbBootRegisters(Mmu& mmu, Registers& regs) {
    regs.Set(Reg8::A, 0x11);
    regs.Set(Reg8::F, 0x80);
    regs.Set(Reg8::B, 0x00);
    regs.Set(Reg8::C, 0x00);
    regs.Set(Reg8::D, 0xff);
    regs.Set(Reg8::E, 0x56);
    regs.Set(Reg8::H, 0x00);
    regs.Set(Reg8::L, 0x0d);
    regs.sp = 0xfffe;
    regs.pc = 0x0100;

    mmu.Write8(std::to_underlying(IO::P1), 0xc7);
    mmu.Write8(std::to_underlying(IO::SB), 0x00);
    mmu.Write8(std::to_underlying(IO::SC), 0x7f);
    mmu.Write8(std::to_underlying(IO::DIV), 0xab);
    mmu.Write8(std::to_underlying(IO::TIMA), 0x00);
    mmu.Write8(std::to_underlying(IO::TMA), 0x00);
    mmu.Write8(std::to_underlying(IO::TAC), 0xf8);
    mmu.Write8(std::to_underlying(IO::IF), 0xE1);
    mmu.Write8(std::to_underlying(IO::NR10), 0x80);
    mmu.Write8(std::to_underlying(IO::NR11), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR12), 0xf3);
    mmu.Write8(std::to_underlying(IO::NR13), 0xff);
    mmu.Write8(std::to_underlying(IO::NR14), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR21), 0x3f);
    mmu.Write8(std::to_underlying(IO::NR22), 0x00);
    mmu.Write8(std::to_underlying(IO::NR23), 0xff);
    mmu.Write8(std::to_underlying(IO::NR24), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR30), 0x7f);
    mmu.Write8(std::to_underlying(IO::NR31), 0xff);
    mmu.Write8(std::to_underlying(IO::NR32), 0x9f);
    mmu.Write8(std::to_underlying(IO::NR33), 0xff);
    mmu.Write8(std::to_underlying(IO::NR34), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR41), 0xff);
    mmu.Write8(std::to_underlying(IO::NR42), 0x00);
    mmu.Write8(std::to_underlying(IO::NR43), 0x00);
    mmu.Write8(std::to_underlying(IO::NR44), 0xbf);
    mmu.Write8(std::to_underlying(IO::NR50), 0x77);
    mmu.Write8(std::to_underlying(IO::NR51), 0xf3);
    mmu.Write8(std::to_underlying(IO::NR52), 0xf1);
    mmu.Write8(std::to_underlying(IO::LCDC), 0x91);
    mmu.Write8(std::to_underlying(IO::STAT), 0x85);
    mmu.Write8(std::to_underlying(IO::SCY), 0x00);
    mmu.Write8(std::to_underlying(IO::SCX), 0x00);
    mmu.Write8(std::to_underlying(IO::LY), 0x91);
    mmu.Write8(std::to_underlying(IO::LYC), 0x00);
    mmu.Write8(std::to_underlying(IO::DMA), 0x00);
    mmu.Write8(std::to_underlying(IO::BGP), 0xfc);
    mmu.Write8(std::to_underlying(IO::OBP0), 0xff);
    mmu.Write8(std::to_underlying(IO::OBP1), 0xff);
    mmu.Write8(std::to_underlying(IO::WY), 0x00);
    mmu.Write8(std::to_underlying(IO::WX), 0x00);
    mmu.Write8(std::to_underlying(IO::KEY0), 0x00);
    mmu.Write8(std::to_underlying(IO::KEY1), 0x7e);
    mmu.Write8(std::to_underlying(IO::VBK), 0xfe);
    mmu.Write8(std::to_underlying(IO::HDMA1), 0xff);
    mmu.Write8(std::to_underlying(IO::HDMA2), 0xff);
    mmu.Write8(std::to_underlying(IO::HDMA3), 0xff);
    mmu.Write8(std::to_underlying(IO::HDMA4), 0xff);
    mmu.Write8(std::to_underlying(IO::HDMA5), 0xff);
    mmu.Write8(std::to_underlying(IO::RP), 0x3e);
    mmu.Write8(std::to_underlying(IO::BCPS), 0x00);
    mmu.Write8(std::to_underlying(IO::BCPD), 0x00);
    mmu.Write8(std::to_underlying(IO::OCPS), 0x00);
    mmu.Write8(std::to_underlying(IO::OCPD), 0x00);
    mmu.Write8(std::to_underlying(IO::SVBK), 0xf8);
    mmu.Write8(std::to_underlying(IO::IE), 0x00);
}

void Emulator::Init(EmulatorConfig emu_cfg) {
  config_ = std::move(emu_cfg);

  sample_bufffer_.resize(config_.num_channels * config_.buffer_size);

  timer_.Init({
    .interrupts = &interrupts_,
  });

  ppu_.Init({
    .mmu = &mmu_,
    .interrupts = &interrupts_,
    .palette = config_.palette,
  });

  audio_.Init({
    .clock_speed = config_.clock_speed,
    .buffer_size = config_.buffer_size,
    .num_channels = config_.num_channels,
    .sample_rate = config_.sample_rate,
  });

  mmu_.Init();
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

  cpu_.Init({
    .mmu = &mmu_,
    .interrupts = &interrupts_,
  });
  cpu_.AddSyncedDevice(&timer_);
  cpu_.AddSyncedDevice(&ppu_);
  cpu_.AddSyncedDevice(&audio_);
  cpu_.AddSyncedDevice(&serial_device_);

  input_device_.Init({
    .interrupts = &interrupts_,
  });

  serial_device_.Init({
    .interrupts = &interrupts_,
  });
  serial_device_.OnLine([] (std::string_view str) {
    spdlog::info("Serial: {}", str);
  });
}

void Emulator::Update(float dt) {
  ZoneScoped;

  static int current_cycles = 0;
  const auto target_cycles_per_frame = static_cast<int>(config_.clock_speed / config_.frame_rate);

  int prev_cycles = current_cycles;
  do {
    auto cycles = cpu_.Execute();
    current_cycles += cycles;
    num_cycles_ += cycles;

    if (breakpoints_.contains(cpu_.GetRegisters().pc)) {
      running_ = false;
      break;
    }
  } while (current_cycles < target_cycles_per_frame);
  prev_cycles_ = current_cycles - prev_cycles;
  current_cycles -= target_cycles_per_frame;

  ppu_.UpdateRenderTargets();
}

void Emulator::Cleanup() {
  ppu_.Cleanup();
}

void Emulator::LoadCartBytes(std::vector<u8> bytes) {
  cart_bytes_ = std::move(bytes);
  Reset();
}

void Emulator::ClearCartBytes() {
  cart_bytes_.clear();
  Reset();
}

bool Emulator::IsCartLoaded() const {
  return !cart_bytes_.empty();
}

void Emulator::Reset() {
  prev_cycles_ = 0;
  num_cycles_ = 0;
  running_ = false;

  hardware_mode_ = HardwareMode::kDmgMode;

  cpu_.Reset();
  mmu_.ResetDevices();
  cart_.LoadCartBytes(cart_bytes_);

  auto& cart_info = cart_.GetCartridgeInfo();
  if (mode_ == EmulationMode::kAutoMode) {
    switch (cart_info.cgb_flag) {
      case CgbFlag::kCgbCompatMode:
      case CgbFlag::kCgbOnlyMode:
        hardware_mode_ = HardwareMode::kCgbMode;
        break;
      default:
        hardware_mode_ = HardwareMode::kDmgMode;
        break;
    }
  } else {
    const auto val = magic_enum::enum_integer(mode_);
    hardware_mode_ = magic_enum::enum_cast<HardwareMode>(val).value_or(HardwareMode::kDmgMode);
  }

  spdlog::info("Internal emulation mode: {}", magic_enum::enum_name(hardware_mode_));

  const auto& boot_rom = boot_roms_.at(hardware_mode_);

  spdlog::debug("Using boot rom type:{} at '{}'", magic_enum::enum_name(hardware_mode_), boot_rom.path);

  boot_.LoadBytes(boot_rom.data);

  if (skip_bootrom_) {
    if (hardware_mode_ == HardwareMode::kCgbMode) {
      SetCgbBootRegisters(mmu_, cpu_.GetRegisters());
    } else {
      SetDmgBootRegisters(mmu_, cpu_.GetRegisters());
    }
    boot_.SetDisable(0xff);
  }
}

void Emulator::Step() {
  if (running_) {
    return;
  }
  num_cycles_ += cpu_.Execute();
  ppu_.UpdateRenderTargets();
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
  return cpu_.GetRegisters();
}

const State& Emulator::GetState() const {
  return cpu_.GetState();
}

size_t Emulator::GetTotalCycles() const {
  return num_cycles_;
}

PPUMode Emulator::GetMode() const {
  return ppu_.GetMode();
}

Instruction Emulator::GetCurrentInstruction() const {
  const auto& regs = cpu_.GetRegisters();
  auto byte = mmu_.Read8(regs.pc);
  auto instr = Decoder::Decode(byte);
  if (instr.opcode == Opcode::PREFIX) {
    byte = mmu_.Read8(regs.pc + 1);
    instr = Decoder::DecodePrefixed(byte);
  }
  return instr;
}

u8 Emulator::Read8(u16 addr) const {
  return mmu_.Read8(addr);
}

u16 Emulator::Read16(u16 addr) const {
  u8 lo = mmu_.Read8(addr);
  u8 hi = mmu_.Read8(addr + 1);
  return lo | (hi << 8);
}

void Emulator::Write8(u16 addr, u8 byte) {
  mmu_.Write8(addr, byte);
}

const Texture2D& Emulator::GetTargetLCD() const {
  return ppu_.GetTextureLcd();
}

const RenderTexture2D& Emulator::GetTargetTiles() const {
  return ppu_.GetTextureTiles();
}

const RenderTexture2D& Emulator::GetTargetTilemap(u8 idx) const {
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

void Emulator::AddBreakPoint(u16 addr) {
  breakpoints_.insert(addr);
}

void Emulator::RemoveBreakPoint(u16 addr) {
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

void Emulator::OnAudioCallback(std::span<float> buffer) {
  if (IsCartLoaded() && IsPlaying()) {
    audio_.GetSamples(buffer);
  } else {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
  }
}

std::expected<void, std::string> Emulator::SetBootRomPath(HardwareMode mode, std::string_view path) {
  auto result = file::LoadBin(path);
  if (!result) {
    return std::unexpected{result.error()};
  }

  boot_roms_[mode] = BootRomData{
    .data = result.value(),
    .path = std::string(path),
  };

  return {};
}

std::string Emulator::GetBootRomPath(HardwareMode mode) const {
  auto it = boot_roms_.find(mode);
  if (it == boot_roms_.end()) {
    return {};
  }
  return it->second.path;
}

size_t Emulator::GetPrevCycles() const {
  return prev_cycles_;
}

void Emulator::ResetFrameCount() {
  ppu_.ResetFrameCount();
}

size_t Emulator::GetFrameCount() const {
  return ppu_.GetFrameCount();
}

void Emulator::UpdatePalette(std::array<Color, 4> palette) {
  ppu_.UpdatePalette(std::move(palette));
}

void Emulator::SetClockSpeed(size_t clock_speed) {
  config_.clock_speed = clock_speed;
}

size_t Emulator::GetClockSpeed() const {
  return config_.clock_speed;
}

void Emulator::SetEmulationMode(EmulationMode mode) {
  mode_ = mode;
}

EmulationMode Emulator::GetEmulationMode() const {
  return mode_;
}
