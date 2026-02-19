#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "cart_device.hpp"
#include "cart_header.hpp"
#include "no_mbc.hpp"
#include "mbc1.hpp"
#include "mbc2.hpp"
#include "mbc3.hpp"
#include "mbc5.hpp"

bool CartDevice::IsValidFor(uint16_t addr) const {
  return addr <= kRomBank01End || (addr >= kExtRamStart && addr <= kExtRamEnd);
}

void CartDevice::Write8(uint16_t addr, uint8_t byte) {
  if (addr >= kExtRamStart && addr <= kExtRamEnd) {
    return mbc_->WriteRam(addr, byte);
  }
  return mbc_->WriteReg(addr, byte);
}

uint8_t CartDevice::Read8(uint16_t addr) const {
  if (addr <= kRomBank00End) {
    return mbc_->ReadRom0(addr);
  }

  if (addr <= kRomBank01End) {
    return mbc_->ReadRom1(addr);
  }

  return mbc_->ReadRam(addr);
}

void CartDevice::Reset() {
  info_.Reset();
  mbc_ = std::make_unique<NoMbc>();
}

void CartDevice::LoadCartBytes(const std::vector<uint8_t> &bytes) {
  if (bytes.empty()) {
    return Reset();
  }

  const auto *rom_base = bytes.data();
  std::string title { rom_base + 0x0134, std::find(rom_base + 0x0134, rom_base + 0x0144, 0) };
  CartType cart_type { *(rom_base + 0x0147) };
  size_t rom_size_kb = 32 * (1 << *(rom_base + 0x0148));
  size_t rom_banks = rom_size_kb / 16;
  RamType ram_type { *(rom_base + 0x0149) };

  size_t ram_banks;
  size_t ram_size;
  switch (ram_type) {
    case RamType::UNUSED:
      ram_size = 2048;
      ram_banks = 0;
      break;
    case RamType::BANKS_1:
      ram_size = 1;
      ram_banks = 1;
      break;
    case RamType::BANKS_4:
      ram_size = 32768;
      ram_banks = 4;
      break;
    case RamType::BANKS_8:
      ram_size = 65536;
      ram_banks = 8;
      break;
    case RamType::BANKS_16:
      ram_size = 131072;
      ram_banks = 16;
      break;
    default:
      ram_size = 0;
      ram_banks = 0;
  }

  info_.title = title;
  info_.type = cart_type;
  info_.rom_size_bytes = rom_size_kb * 1024;
  info_.rom_num_banks = rom_banks;
  info_.ram_type = ram_type;
  info_.ram_num_banks = ram_banks;
  info_.ram_size_bytes = ram_size;

  spdlog::info("Loaded cartridge");
  spdlog::info("Title: {}", title);
  spdlog::info("Cart type: {}", magic_enum::enum_name(cart_type));
  spdlog::info("ROM size: {}KiB, No. Banks: {}", rom_size_kb, rom_banks);
  spdlog::info("RAM: {}", magic_enum::enum_name(ram_type));

  bool has_ram = false;
  bool has_battery = false;

  switch (cart_type) {
    case CartType::ROM_ONLY:
    case CartType::ROM_RAM:
    case CartType::ROM_RAM_BATTERY:
      mbc_ = std::make_unique<NoMbc>(bytes);
      break;
    case CartType::MBC1_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC1_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC1:
      mbc_ = std::make_unique<Mbc1>(bytes, info_, has_ram, has_battery);
      break;
    case CartType::MBC2_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC2:
      mbc_ = std::make_unique<Mbc2>(bytes, info_, has_ram, has_battery);
      break;
    case CartType::MBC3_TIMER_RAM_BATTERY:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC3_TIMER_BATTERY:
      mbc_ = std::make_unique<Mbc3>(bytes, info_, has_ram, true, true);
      break;
    case CartType::MBC3_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC3_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC3:
      mbc_ = std::make_unique<Mbc3>(bytes, info_, has_ram, has_battery, false);
      break;
    case CartType::MBC5_RUMBLE_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC5_RUMBLE_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC5_RUMBLE:
      mbc_ = std::make_unique<Mbc5>(bytes, info_, has_ram, has_battery, true);
      break;
    case CartType::MBC5_RAM_BATTERY:
      has_battery = true;
      [[fallthrough]];
    case CartType::MBC5_RAM:
      has_ram = true;
      [[fallthrough]];
    case CartType::MBC5:
      mbc_ = std::make_unique<Mbc5>(bytes, info_, has_ram, has_battery, false);
      break;
    case CartType::MMM01:
    case CartType::MMM01_RAM:
    case CartType::MMM01_RAM_BATTERY:
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

const CartInfo& CartDevice::GetCartridgeInfo() const {
  return info_;
}
