#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "cart_device.h"
#include "cart_header.h"
#include "mbc1.h"

bool CartDevice::valid_for(uint16_t addr) const {
  return addr <= kRomBank01End || (addr >= kExtRamStart && addr <= kExtRamEnd);
}

void CartDevice::write8(uint16_t addr, uint8_t byte) {
  if (mbc) {
    mbc->write8(addr, byte);
    return;
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    ext_ram[addr - kExtRamStart] = byte;
  }
}

uint8_t CartDevice::read8(uint16_t addr) const {
  if (mbc) {
    return mbc->read8(addr);
  }

  if (addr <= kRomBank01End && addr < cart_rom.size()) {
    return cart_rom[addr];
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    return ext_ram[addr - kExtRamStart];
  }

  return 0;
}

void CartDevice::reset() {
  cart_rom.clear();
  ext_ram.fill(0);
  info.reset();
  mbc = nullptr;
}

void CartDevice::load_cartridge(const std::vector<uint8_t> &bytes) {
  cart_rom = bytes;
  if (cart_rom.empty()) {
    return;
  }

  cart_header *header = reinterpret_cast<cart_header *>(&*cart_rom.begin() + 0x100);
  CartType cart_type { header->cartridge_type };
  auto null_marker = std::find(header->title.begin(), header->title.end(), 0);
  std::string title { header->title.begin(), null_marker };
  RamType ram_type { header->ram_size };
  size_t rom_size_kb = 32 * (1 << header->rom_size);
  size_t rom_banks = rom_size_kb / 16;

  info = {
    title,
    cart_type,
    rom_size_kb * 1024,
    rom_banks,
    ram_type,
  };

  spdlog::info("Loaded cartridge");
  spdlog::info("Title: {}", title);
  spdlog::info("Cart type: {}", magic_enum::enum_name(cart_type));
  spdlog::info("ROM size: {}KiB, No. Banks: {}", rom_size_kb, rom_banks);
  spdlog::info("RAM: {}", magic_enum::enum_name(ram_type));

  bool has_ram = false;
  bool has_battery = false;
  bool has_timer = false;
  bool has_rumble = false;

  switch (cart_type) {
    case CartType::ROM_ONLY: break;
    case CartType::MBC1_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC1_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC1:
      mbc = std::make_unique<Mbc1>(cart_rom, info, has_ram, has_battery);
      break;
    case CartType::MBC2:
    case CartType::MBC2_BATTERY:
    case CartType::ROM_RAM:
    case CartType::ROM_RAM_BATTERY:
    case CartType::MMM01:
    case CartType::MMM01_RAM:
    case CartType::MMM01_RAM_BATTERY:
    case CartType::MBC3_TIMER_BATTERY:
    case CartType::MBC3_TIMER_RAM_BATTERY:
    case CartType::MBC3:
    case CartType::MBC3_RAM:
    case CartType::MBC3_RAM_BATTERY:
    case CartType::MBC5:
    case CartType::MBC5_RAM:
    case CartType::MBC5_RAM_BATTERY:
    case CartType::MBC5_RUMBLE:
    case CartType::MBC5_RUMBLE_RAM:
    case CartType::MBC5_RUMBLE_RAM_BATTERY:
    case CartType::MBC6:
    case CartType::MBC7_SENSOR_RUMBLE_RAM_BATTERY:
    case CartType::POCKET_CAMERA:
    case CartType::BANDAI_TAMA5:
    case CartType::HUC3:
    case CartType::HUC1_RAM_BATTERY:
    default:
      spdlog::error("Cart type not yet implemented!: {}", magic_enum::enum_name(cart_type));
      std::unreachable();
  }
}

const cart_info &CartDevice::cartridge_info() const {
  return info;
}
