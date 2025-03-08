#include <array>
#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "file.h"
#include "mmu.h"

// NOTE: not sure why, but my emulator seems to be 4x slower than it should be...
constexpr auto kStaticSpeedMultiplier = 4;

Emulator::Emulator():cpu{mmu, interrupts}, ppu{mmu, interrupts}, serial_device{interrupts}, timer{interrupts}, input_device{interrupts}, audio{timer} {
  serial_device.on_line([] (const std::string &str) {
    spdlog::info("Serial: {}", str);
  });
}

tl::expected<bool, std::string> Emulator::init() {
  ppu.init();

  mmu.clear_devices();
  mmu.add_device(&boot);
  mmu.add_device(&cart);
  mmu.add_device(&wram);
  mmu.add_device(&ppu);
  mmu.add_device(&hram);
  mmu.add_device(&audio);
  mmu.add_device(&timer);
  mmu.add_device(&input_device);
  mmu.add_device(&serial_device);
  mmu.add_device(&interrupts);
  mmu.add_device(&null_device);

  cpu.add_synced(&timer);
  cpu.add_synced(&ppu);
  cpu.add_synced(&audio);
  cpu.add_synced(&serial_device);

  auto result = load_bin("./boot.bin");
  if (!result) {
    return tl::unexpected(fmt::format("Failed to load boot rom: {}", result.error()));
  }

  const auto &bytes = result.value();
  boot_rom.fill(0);
  std::copy_n(bytes.begin(), std::min(bytes.size(), boot_rom.size()), boot_rom.begin());

  return true;
}

void Emulator::update(float dt) {
  if (!running && !cpu.state.halt) {
    return;
  }

  const auto target_cycles_per_frame = static_cast<int>(kClockSpeed * dt) * kStaticSpeedMultiplier;
  static int current_cycles = 0;

  do {
    auto cycles = cpu.execute();
    current_cycles += cycles;
    num_cycles += cycles;

    if (breakpoints.contains(cpu.regs.pc)) {
      running = false;
      break;
    }
  } while (current_cycles < target_cycles_per_frame);
  current_cycles -= target_cycles_per_frame;

  ppu.update_render_targets();
}

void Emulator::cleanup() {
  ppu.cleanup();
}

void Emulator::load_cartridge(std::vector<uint8_t> &&bytes) {
  cart_bytes = std::move(bytes);
  reset();
}

void Emulator::reset() {
  num_cycles = 0;
  running = false;

  cpu.reset();
  mmu.reset_devices();
  boot.load_bytes(boot_rom);
  cart.load_cartridge(cart_bytes);

  if (_skip_bootrom) {
    cpu.regs.set(Reg8::A, 0x01);
    cpu.regs.set(Reg8::F, 0xb0);
    cpu.regs.set(Reg8::B, 0x00);
    cpu.regs.set(Reg8::C, 0x13);
    cpu.regs.set(Reg8::D, 0x00);
    cpu.regs.set(Reg8::E, 0xd8);
    cpu.regs.set(Reg8::H, 0x01);
    cpu.regs.set(Reg8::L, 0x4d);
    cpu.regs.sp = 0xfffe;
    cpu.regs.pc = 0x0100;

    mmu.write8(std::to_underlying(IO::P1), 0xcf);
    mmu.write8(std::to_underlying(IO::SB), 0x00);
    mmu.write8(std::to_underlying(IO::SC), 0x7e);
    mmu.write8(std::to_underlying(IO::DIV), 0xab);
    mmu.write8(std::to_underlying(IO::TIMA), 0x00);
    mmu.write8(std::to_underlying(IO::TMA), 0x00);
    mmu.write8(std::to_underlying(IO::TAC), 0xf8);
    mmu.write8(std::to_underlying(IO::IF), 0xE1);
    mmu.write8(std::to_underlying(IO::NR10), 0x80);
    mmu.write8(std::to_underlying(IO::NR11), 0xbf);
    mmu.write8(std::to_underlying(IO::NR12), 0xf3);
    mmu.write8(std::to_underlying(IO::NR13), 0xff);
    mmu.write8(std::to_underlying(IO::NR14), 0xbf);
    mmu.write8(std::to_underlying(IO::NR21), 0x3f);
    mmu.write8(std::to_underlying(IO::NR22), 0x00);
    mmu.write8(std::to_underlying(IO::NR23), 0xff);
    mmu.write8(std::to_underlying(IO::NR24), 0xbf);
    mmu.write8(std::to_underlying(IO::NR30), 0x7f);
    mmu.write8(std::to_underlying(IO::NR31), 0xff);
    mmu.write8(std::to_underlying(IO::NR32), 0x9f);
    mmu.write8(std::to_underlying(IO::NR33), 0xff);
    mmu.write8(std::to_underlying(IO::NR34), 0xbf);
    mmu.write8(std::to_underlying(IO::NR41), 0xff);
    mmu.write8(std::to_underlying(IO::NR42), 0x00);
    mmu.write8(std::to_underlying(IO::NR43), 0x00);
    mmu.write8(std::to_underlying(IO::NR44), 0xbf);
    mmu.write8(std::to_underlying(IO::NR50), 0x77);
    mmu.write8(std::to_underlying(IO::NR51), 0xf3);
    mmu.write8(std::to_underlying(IO::NR52), 0xf0);
    mmu.write8(std::to_underlying(IO::LCDC), 0x91);
    mmu.write8(std::to_underlying(IO::STAT), 0x85);
    mmu.write8(std::to_underlying(IO::LYC), 0x00);
    mmu.write8(std::to_underlying(IO::DMA), 0xff);
    mmu.write8(std::to_underlying(IO::BGP), 0xfc);
    mmu.write8(std::to_underlying(IO::WX), 0x00);
    mmu.write8(std::to_underlying(IO::WY), 0x00);
    mmu.write8(std::to_underlying(IO::IE), 0x00);

    boot.set_disable(0xff);
  }
}

void Emulator::step() {
  if (running) {
    return;
  }
  num_cycles += cpu.execute();
}

void Emulator::play() {
  running = true;
}

void Emulator::stop() {
  running = false;
}

bool Emulator::is_playing() const {
  return running;
}

const Registers& Emulator::registers() const {
  return cpu.regs;
}

const State& Emulator::state() const {
  return cpu.state;
}

size_t Emulator::cycles() const {
  return num_cycles;
}

PPUMode Emulator::mode() const {
  return ppu.mode();
}

Instruction Emulator::instr() const {
  auto byte = mmu.read8(cpu.regs.pc);
  auto instr = Decoder::decode(byte);
  if (instr.opcode == Opcode::PREFIX) {
    byte = mmu.read8(cpu.regs.pc + 1);
    instr = Decoder::decode_prefixed(byte);
  }
  return instr;
}

uint8_t Emulator::read8(uint16_t addr) const {
  return mmu.read8(addr);
}

const Texture2D& Emulator::target_lcd() const {
  return ppu.lcd();
}

const RenderTexture2D& Emulator::target_tiles() const {
  return ppu.tiles();
}

const RenderTexture2D& Emulator::target_tilemap(uint8_t idx) const {
  if (idx == 0) {
    return ppu.tilemap1();
  } else if (idx == 1) {
    return ppu.tilemap2();
  }
  std::unreachable();
}

const RenderTexture2D& Emulator::target_sprites() const {
  return ppu.sprites();
}

void Emulator::add_breakpoint(uint16_t addr) {
  breakpoints.insert(addr);
}

void Emulator::remove_breakpoint(uint16_t addr) {
  breakpoints.erase(addr);
}

void Emulator::clear_breakpoints() {
  breakpoints.clear();
}

void Emulator::update_input(JoypadButton btn, bool pressed) {
  input_device.update(btn, pressed);
}

void Emulator::set_skip_bootrom(bool skip) {
  _skip_bootrom = skip;
}

bool Emulator::skip_bootrom() const {
  return _skip_bootrom;
}

void Emulator::audio_samples(float *samples, size_t samples_size, size_t num_channels) {
  audio.get_samples(samples, samples_size, num_channels);
}
