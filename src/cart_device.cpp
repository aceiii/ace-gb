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
    if (addr <= kRomBank01End) {
      mbc->write_reg(addr, byte);
    } else if (addr >= kExtRamStart && addr <= kExtRamEnd) {
      mbc->write_ram(addr, byte);
    }
    return;
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    ext_ram[addr - kExtRamStart] = byte;
  }
}

uint8_t CartDevice::read8(uint16_t addr) const {
  if (mbc) {
    if (addr <= kRomBank00End) {
      return mbc->read_rom0(addr);
    }

    if (addr >= kRomBank01Start && addr <= kRomBank01End) {
      return mbc->read_rom1(addr);
    }

    if (addr >= kExtRamStart && addr <= kExtRamEnd) {
      return mbc->read_ram(addr);
    }

    return 0xff;
  }

  if (addr <= kRomBank01End && addr < cart_rom.size()) {
    return cart_rom[addr];
  }

  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    return ext_ram[addr - kExtRamStart];
  }

  return 0xff;
}

void CartDevice::reset() {
  cart_rom.fill(0);
  ext_ram.fill(0);
  info.reset();
  mbc = nullptr;
}

void CartDevice::load_cartridge(const std::vector<uint8_t> &bytes) {
  if (bytes.empty()) {
    reset();
    return;
  }

  const auto *rom_base = bytes.data();
  std::string title { rom_base + 0x0134, std::find(rom_base + 0x0134, rom_base + 0x0144, 0) };
  CartType cart_type { *(rom_base + 0x0147) };
  size_t rom_size_kb = 32 * (1 << *(rom_base + 0x0148));
  size_t rom_banks = rom_size_kb / 16;
  RamType ram_type { *(rom_base + 0x0149) };

  size_t ram_banks;
  switch (ram_type) {
    case RamType::BANKS_1: ram_banks = 1; break;
    case RamType::BANKS_4: ram_banks = 4; break;
    case RamType::BANKS_8: ram_banks = 8; break;
    case RamType::BANKS_16: ram_banks = 16; break;
    default: ram_banks = 0;
  }

  info.title = title;
  info.type = cart_type;
  info.rom_size_bytes = rom_size_kb * 1024;
  info.rom_num_banks = rom_banks;
  info.ram_type = ram_type;
  info.ram_num_banks = ram_banks;

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
    case CartType::ROM_ONLY:
      std::copy_n(bytes.begin(), std::min(cart_rom.size(), bytes.size()), cart_rom.begin());
      break;
    case CartType::MBC1_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC1_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC1:
      mbc = std::make_unique<Mbc1>(bytes, info, has_ram, has_battery);
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
