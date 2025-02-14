#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "file.h"
#include "mmu.h"


Emulator::Emulator(bool doctor_log):_doctor_log{doctor_log}, cpu{mmu, interrupts}, ppu{mmu, interrupts}, serial_device{interrupts}, timer{interrupts} {
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

void Emulator::update() {
  if (!running && !cpu.state.halt) {
    return;
  }

  const auto fps = 60;
  const auto cycles_per_frame = kClockSpeed / fps;
  int current_cycles = 0;

  do {
    log_doctor();
    current_cycles += cpu.execute();

    if (breakpoints.contains(cpu.regs.pc)) {
      running = false;
      break;
    }
  } while (current_cycles < cycles_per_frame);

  num_cycles += current_cycles;

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
  }
}

void Emulator::step() {
  if (running) {
    return;
  }

  log_doctor();
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

void Emulator::log_doctor() const {
  auto logger = spdlog::get("doctor_logger");
  if (!logger) {
    return;
  }

  auto a = cpu.regs.get(Reg8::A);
  auto f = cpu.regs.get(Reg8::F);
  auto b = cpu.regs.get(Reg8::B);
  auto c = cpu.regs.get(Reg8::C);
  auto d = cpu.regs.get(Reg8::D);
  auto e = cpu.regs.get(Reg8::E);
  auto h = cpu.regs.get(Reg8::H);
  auto l = cpu.regs.get(Reg8::L);
  auto pc = cpu.regs.pc;
  auto sp = cpu.regs.sp;

  auto mem = std::to_array<uint8_t>({
    cpu.mmu.read8(pc),
    cpu.mmu.read8(pc + 1),
    cpu.mmu.read8(pc + 2),
    cpu.mmu.read8(pc + 3),
  });

  logger->info("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}", a, f, b, c, d, e, h, l, sp, pc, mem[0], mem[1], mem[2], mem[3]);
}
