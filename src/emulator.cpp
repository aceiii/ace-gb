#include <fstream>
#include <memory>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <rlImGui.h>

#include "emulator.h"
#include "file.h"
#include "mmu.h"

void rlImGuiImageTextureFit(const Texture2D *image, bool center)
{
  if (!image)
    return;

  ImVec2 area = ImGui::GetContentRegionAvail();

  float scale =  area.x / image->width;

  float y = image->height * scale;
  if (y > area.y)
  {
    scale = area.y / image->height;
  }

  int sizeX = image->width * scale;
  int sizeY = image->height * scale;

  if (center)
  {
    ImGui::SetCursorPosX(0);
    ImGui::SetCursorPosX(area.x/2 - sizeX/2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (area.y / 2 - sizeY / 2));
  }

  rlImGuiImageRect(image, sizeX, sizeY, Rectangle{ 0,0, static_cast<float>(image->width), static_cast<float>(image->height) });
}

Emulator::Emulator():cpu{mmu, interrupts}, ppu{mmu, interrupts}, serial_device{interrupts}, timer{interrupts} {
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
    auto cycles = cpu.execute();
    timer.execute(cycles);
    ppu.execute(cycles);
    serial_device.execute(cycles);
    current_cycles += cycles;
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
}

void Emulator::step() {
  if (running) {
    return;
  }

  auto cycles = cpu.execute();
  timer.execute(cycles);
  ppu.execute(cycles);
  serial_device.execute(cycles);

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

const State& Emulator::state() const {
  return cpu.state;
}

size_t Emulator::cycles() const {
  return num_cycles;
}

void Emulator::render(bool &show_window) {
  ImGui::SetNextWindowSize({ 300, 300 }, ImGuiCond_FirstUseEver);
  ImGui::Begin("GameBoy", &show_window);
  {
    rlImGuiImageTextureFit(&ppu.lcd(), true);
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

const RenderTexture2D &Emulator::target_tiles() const {
  return ppu.tiles();
}

const RenderTexture2D &Emulator::target_tilemap(uint8_t idx) const {
  if (idx == 0) {
    return ppu.tilemap1();
  } else if (idx == 1) {
    return ppu.tilemap2();
  }
  std::unreachable();
}

const RenderTexture2D &Emulator::target_sprites() const {
  return ppu.sprites();
}
