#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "mmu.h"

Emulator::Emulator() {
  mmu = std::make_unique<MMU>();
}

bool Emulator::init() {
  cpu.init();
  ppu.init();

  mmu->on_write8(std::to_underlying(IO::DIV), [](uint16_t addr, uint8_t val) {
    return 0;
  });

  mmu->on_write8(std::to_underlying(IO::TAC), [&](uint16_t addr, uint8_t tac) {
    update_tima(tac);
    return tac;
  });

  std::ifstream input("./boot.bin", std::ios::binary);
  if (input.fail()) {
    spdlog::error("Failed to load boot rom: {}", strerror(errno));
    return false;
  }

  input.seekg(0, std::ios::end);
  auto boot_rom_size = input.tellg();
  input.seekg(0, std::ios::beg);

  std::vector<uint8_t> rom_bytes;
  rom_bytes.reserve(boot_rom_size);
  input.read((char*)rom_bytes.data(), boot_rom_size);

  spdlog::info("Boot rom[0x09]: {:02x}", rom_bytes[0x09]);
  spdlog::info("Boot rom[0x0A]: {:02x}", rom_bytes[0x0a]);
  spdlog::info("Boot rom[0x0B]: {:02x}", rom_bytes[0x0b]);
  spdlog::info("Boot rom[0x0C]: {:02x}", rom_bytes[0x0c]);
  spdlog::info("Boot rom[0x0D]: {:02x}", rom_bytes[0x0d]);
  spdlog::info("Boot rom[0x0E]: {:02x}", rom_bytes[0x0e]);
  spdlog::info("Boot rom[0x0F]: {:02x}", rom_bytes[0x0f]);

  spdlog::info("Loading boot ROM data.");
  mmu->load_boot_rom(rom_bytes.data());

  for (int i = 0; i < rom_bytes.size(); i++) {
    uint8_t byte = rom_bytes[i];
    mmu->write(i, byte);
  }

  spdlog::info("Starting CPU.");

  return true;
}

void Emulator::update() {
  if (!running && !cpu.state.halt) {
    return;
  }

  auto mmu_ptr = mmu.get();
  const auto fps = 60;
  const auto cycles_per_frame = kClockSpeed / fps;
  int cycles = 0;

  do {
    cycles += cpu.execute(mmu_ptr);
    execute_timers(cycles);
    ppu.execute(mmu_ptr, cycles);
  } while (cycles < cycles_per_frame);

  num_cycles += cycles;
}

void Emulator::cleanup() {
  ppu.cleanup();
}

void Emulator::load_cartridge(const std::vector<uint8_t> &bytes) {
  mmu->load_cartridge(bytes);
}

void Emulator::reset() {
  mmu->reset();
  cpu.reset();
  ppu.reset();

  div_counter = 0;
  tima_counter = 0;
}

void Emulator::step() {
  if (running) {
    return;
  }

  auto mmu_ptr = mmu.get();
  auto cycles = cpu.execute(mmu_ptr);
  execute_timers(cycles);
  ppu.execute(mmu_ptr, cycles);

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

void Emulator::execute_timers(uint8_t cycles) {
  div_counter += cycles;
  if (div_counter >= 256) {
    div_counter -= 256;
    mmu->inc(std::to_underlying(IO::DIV));
  }

  auto tac = mmu->read8(std::to_underlying(IO::TAC));
  if ((tac >> 2) & 0x1) {
    tima_counter += cycles;
    update_tima(tac);
  }

  current_tac = tac;
}

void Emulator::update_tima(uint8_t tac) {
  static uint16_t timer_freqs[] = {
    kClockSpeed / 4096,
    kClockSpeed / 262144,
    kClockSpeed / 65535,
    kClockSpeed / 16384,
  };

  auto freq = timer_freqs[tac & 0x3];
  if (tima_counter >= freq) {
    uint8_t tima = mmu->read8(std::to_underlying(IO::TIMA));
    if (tima == 255) {
      mmu->write(std::to_underlying(IO::TIMA), mmu->read8(std::to_underlying(IO::TMA)));
      uint8_t disabled_bits = mmu->read8(std::to_underlying(IO::IE)) & ~(1 << std::to_underlying(Interrupt::Timer));
      mmu->write(std::to_underlying(IO::IE), disabled_bits);
    } else {
      tima += 1;
      mmu->write(std::to_underlying(IO::TIMA), tima);
    }
    tima_counter -= freq;
  }
};

PPUMode Emulator::mode() const {
  auto lcd_status = mmu->read8(std::to_underlying(IO::STAT));
  PPUMode mode {lcd_status & std::to_underlying(LCDStatusMask::PPUMode)};
  return mode;
}

IMMU* Emulator::mmu_ptr() const {
  return mmu.get();
}

Instruction Emulator::instr() const {
  auto byte = mmu->read8(cpu.regs.pc);
  auto instr = Decoder::decode(byte);
  if (instr.opcode == Opcode::PREFIX) {
    byte = mmu->read8(cpu.regs.pc + 1);
    instr = Decoder::decode_prefixed(byte);
  }
  return instr;
}
