#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "file.h"
#include "mmu.h"

Emulator::Emulator():cpu{mmu} {
}

tl::expected<bool, std::string> Emulator::init() {
  ppu.init();

  mmu.clear_devices();
  mmu.add_device(&boot);
  mmu.add_device(&cart);
  mmu.add_device(&wram);
  mmu.add_device(&ppu);
  mmu.add_device(&timer);

  auto result = load_bin("./boot.bin");
  if (!result) {
    return tl::unexpected(fmt::format("Failed to load boot rom: {}", result.error()));
  }

  const auto &bytes = result.value();
  boot_rom.fill(0);
  std::copy_n(bytes.begin(), std::min(bytes.size(), boot_rom.size()), boot_rom.begin());

  return true;
}

void Emulator::update() {
  if (!running && !cpu.state.halt) {
    return;
  }

  const auto fps = 60;
  const auto cycles_per_frame = kClockSpeed / fps;
  int cycles = 0;

  do {
    cycles += cpu.execute();
    timer.execute(cycles);
    ppu.execute(cycles);
  } while (cycles < cycles_per_frame);

  num_cycles += cycles;
}

void Emulator::cleanup() {
  ppu.cleanup();
}

void Emulator::load_cartridge(const std::vector<uint8_t> &bytes) {
  cart_bytes = bytes;
  reset();
}

void Emulator::reset() {
  num_cycles = 0;
  running = false;

  cpu.reset();
  mmu.reset_devices();
  boot.load_bytes(boot_rom);
  cart.load_cartridge(cart_bytes);
}

void Emulator::step() {
  if (running) {
    return;
  }

  auto cycles = cpu.execute();
  timer.execute(cycles);
  ppu.execute(cycles);

  num_cycles += cycles;
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

size_t Emulator::cycles() const {
  return num_cycles;
}

void Emulator::render() {
  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  ImGui::Begin("GameBoy");
  {
    rlImGuiImageRenderTextureFit(ppu.target(), true);
  }
  ImGui::End();
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
